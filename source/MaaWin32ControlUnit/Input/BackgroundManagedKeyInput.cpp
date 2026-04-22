#include "BackgroundManagedKeyInput.h"

#include <algorithm>
#include <array>
#include <chrono>

#include "MaaUtils/Logger.h"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

constexpr auto guard_interval = std::chrono::milliseconds(5);
constexpr auto apply_timeout = std::chrono::milliseconds(500);
constexpr auto hotkey_wait_timeout = std::chrono::milliseconds(200);
constexpr int managed_hotkey_base = 2000;
constexpr int stop_max_retries = 10;

} // namespace

BackgroundManagedKeyInput::BackgroundManagedKeyInput(HWND hwnd)
    : hwnd_(hwnd)
{
}

BackgroundManagedKeyInput::~BackgroundManagedKeyInput()
{
    {
        std::lock_guard lock(mutex_);
        for (const int keycode : managed_keys_) {
            release_keys_.emplace(keycode);
        }
        managed_keys_.clear();
        desired_pressed_keys_.clear();
        ++desired_generation_;
        stop_thread_ = true;
    }
    guard_cv_.notify_all();
    if (guard_thread_.joinable()) {
        guard_thread_.join();
    }
}

bool BackgroundManagedKeyInput::set_managed_keys(const std::vector<int>& keycodes)
{
    auto normalized = normalize_keycodes(keycodes);

    uint64_t generation = 0;
    bool clear_all = normalized.empty();
    {
        std::lock_guard lock(mutex_);

        if (!thread_started_ && !clear_all) {
            guard_thread_ = std::thread(&BackgroundManagedKeyInput::guard_loop, this);
            thread_started_ = true;
        }

        if (clear_all) {
            for (const int keycode : managed_keys_) {
                release_keys_.emplace(keycode);
                desired_pressed_keys_.erase(keycode);
            }
            managed_keys_.clear();
        }
        else {
            for (const int keycode : managed_keys_) {
                if (!normalized.contains(keycode)) {
                    release_keys_.emplace(keycode);
                    desired_pressed_keys_.erase(keycode);
                }
            }
            managed_keys_ = std::move(normalized);
        }
        generation = ++desired_generation_;
    }

    guard_cv_.notify_all();
    return wait_until_applied(generation);
}

bool BackgroundManagedKeyInput::is_managed_key(int keycode) const
{
    std::lock_guard lock(mutex_);
    return managed_keys_.contains(keycode);
}

bool BackgroundManagedKeyInput::is_key_pressed(int keycode) const
{
    std::lock_guard lock(mutex_);
    return desired_pressed_keys_.contains(keycode);
}

bool BackgroundManagedKeyInput::key_down(int keycode)
{
    if (!is_valid_keycode(keycode)) {
        LogError << "Invalid managed keycode" << VAR(keycode);
        return false;
    }

    bool inserted = false;
    uint64_t generation = 0;
    {
        std::lock_guard lock(mutex_);
        if (!managed_keys_.contains(keycode)) {
            LogError << "Key is not in background managed domain" << VAR(keycode);
            return false;
        }

        auto [_, was_inserted] = desired_pressed_keys_.emplace(keycode);
        inserted = was_inserted;
        if (!inserted) {
            return true;
        }
        generation = ++desired_generation_;
    }

    guard_cv_.notify_all();
    return wait_until_applied(generation);
}

bool BackgroundManagedKeyInput::key_up(int keycode)
{
    if (!is_valid_keycode(keycode)) {
        LogError << "Invalid managed keycode" << VAR(keycode);
        return false;
    }

    uint64_t generation = 0;
    {
        std::lock_guard lock(mutex_);
        if (!managed_keys_.contains(keycode)) {
            LogError << "Key is not in background managed domain" << VAR(keycode);
            return false;
        }

        if (!desired_pressed_keys_.erase(keycode)) {
            return true;
        }
        generation = ++desired_generation_;
    }

    guard_cv_.notify_all();
    return wait_until_applied(generation);
}

bool BackgroundManagedKeyInput::inactive()
{
    uint64_t generation = 0;
    bool has_work = false;
    {
        std::lock_guard lock(mutex_);
        has_work = !managed_keys_.empty() || !desired_pressed_keys_.empty() || !release_keys_.empty();
        if (!has_work) {
            return true;
        }

        for (const int keycode : managed_keys_) {
            release_keys_.emplace(keycode);
        }
        managed_keys_.clear();
        desired_pressed_keys_.clear();
        generation = ++desired_generation_;
    }

    guard_cv_.notify_all();
    return wait_until_applied(generation);
}

std::unordered_set<int> BackgroundManagedKeyInput::normalize_keycodes(const std::vector<int>& keycodes)
{
    std::unordered_set<int> normalized;
    for (const int keycode : keycodes) {
        if (!is_valid_keycode(keycode)) {
            LogError << "Invalid background managed keycode" << VAR(keycode);
            return { };
        }
        normalized.emplace(keycode);
    }
    return normalized;
}

bool BackgroundManagedKeyInput::is_valid_keycode(int keycode)
{
    return keycode > 0 && keycode <= 0xFF;
}

int BackgroundManagedKeyInput::hotkey_id(int keycode)
{
    return managed_hotkey_base + keycode;
}

bool BackgroundManagedKeyInput::is_pressed_now(int keycode)
{
    return (GetAsyncKeyState(keycode) & 0x8000) != 0;
}

void BackgroundManagedKeyInput::send_key_event(int keycode, bool key_up)
{
    INPUT input { };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(keycode);
    input.ki.dwFlags = key_up ? KEYEVENTF_KEYUP : 0;
    SendInput(1, &input, sizeof(INPUT));
}

void BackgroundManagedKeyInput::pump_messages()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void BackgroundManagedKeyInput::send_activation_hint() const
{
    send_activate_message(hwnd_, true);
}

BackgroundManagedKeyInput::Snapshot BackgroundManagedKeyInput::snapshot_locked() const
{
    Snapshot snapshot;
    snapshot.generation = desired_generation_;
    snapshot.desired_pressed_keys = desired_pressed_keys_;
    snapshot.keys.reserve(managed_keys_.size() + release_keys_.size());

    for (const int keycode : managed_keys_) {
        snapshot.keys.emplace_back(keycode);
    }
    for (const int keycode : release_keys_) {
        snapshot.keys.emplace_back(keycode);
        snapshot.release_keys.emplace_back(keycode);
    }

    std::sort(snapshot.keys.begin(), snapshot.keys.end());
    snapshot.keys.erase(std::unique(snapshot.keys.begin(), snapshot.keys.end()), snapshot.keys.end());
    return snapshot;
}

void BackgroundManagedKeyInput::guard_loop()
{
    MSG msg;
    PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE);

    int stop_retries = 0;

    while (true) {
        Snapshot snapshot;
        {
            std::unique_lock lock(mutex_);
            if (managed_keys_.empty() && release_keys_.empty() && !stop_thread_) {
                guard_cv_.wait(lock, [this] { return stop_thread_ || !managed_keys_.empty() || !release_keys_.empty(); });
            }
            else if (!stop_thread_) {
                guard_cv_.wait_for(lock, guard_interval);
            }

            if (stop_thread_ && release_keys_.empty()) {
                break;
            }

            if (stop_thread_) {
                ++stop_retries;
                if (stop_retries > stop_max_retries) {
                    LogWarn << "Exceeded max retries releasing keys on stop, forcing exit" << VAR(release_keys_.size());
                    release_keys_.clear();
                    break;
                }
            }

            snapshot = snapshot_locked();
        }

        bool snapshot_applied = true;
        if (!snapshot.keys.empty()) {
            snapshot_applied = correct_snapshot(snapshot);
        }
        else {
            pump_messages();
        }

        if (snapshot_applied) {
            {
                std::lock_guard lock(mutex_);
                applied_generation_ = std::max(applied_generation_, snapshot.generation);
                for (const int keycode : snapshot.release_keys) {
                    release_keys_.erase(keycode);
                }
            }
            applied_cv_.notify_all();
        }
    }
}

bool BackgroundManagedKeyInput::correct_snapshot(const Snapshot& snapshot)
{
    bool all_applied = true;
    for (const int keycode : snapshot.keys) {
        const bool desired_pressed = snapshot.desired_pressed_keys.contains(keycode);
        all_applied &= ensure_key_state(keycode, desired_pressed);
    }
    pump_messages();
    return all_applied;
}

bool BackgroundManagedKeyInput::ensure_key_state(int keycode, bool desired_pressed)
{
    return desired_pressed ? ensure_key_pressed(keycode) : ensure_key_released(keycode);
}

bool BackgroundManagedKeyInput::ensure_key_pressed(int keycode)
{
    if (is_pressed_now(keycode)) {
        return true;
    }

    send_activation_hint();

    const int id = hotkey_id(keycode);
    const bool registered = RegisterHotKey(nullptr, id, 0, static_cast<UINT>(keycode)) != 0;
    if (!registered) {
        LogWarn << "RegisterHotKey failed for managed key, using fallback injection" << VAR(keycode) << VAR(GetLastError());
        send_key_event(keycode, false);
        pump_messages();
        const bool pressed = is_pressed_now(keycode);
        return pressed;
    }

    send_key_event(keycode, false);

    MSG msg;
    int extra_count = 0;
    const auto deadline = std::chrono::steady_clock::now() + hotkey_wait_timeout;
    bool got_ours = false;
    while (std::chrono::steady_clock::now() < deadline) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY && static_cast<int>(msg.wParam) == id) {
                if (!got_ours) {
                    got_ours = true;
                }
                else {
                    ++extra_count;
                }
            }
            else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        if (got_ours) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    UnregisterHotKey(nullptr, id);

    while (PeekMessageW(&msg, nullptr, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {
        if (static_cast<int>(msg.wParam) == id) {
            ++extra_count;
        }
    }

    for (int i = 0; i < extra_count; ++i) {
        std::array<INPUT, 2> replay { };
        replay[0].type = INPUT_KEYBOARD;
        replay[0].ki.wVk = static_cast<WORD>(keycode);
        replay[1].type = INPUT_KEYBOARD;
        replay[1].ki.wVk = static_cast<WORD>(keycode);
        replay[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(static_cast<UINT>(replay.size()), replay.data(), sizeof(INPUT));
        pump_messages();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (extra_count > 0 && !is_pressed_now(keycode)) {
        send_key_event(keycode, false);
        pump_messages();
    }

    const bool pressed = is_pressed_now(keycode);
    return pressed;
}

bool BackgroundManagedKeyInput::ensure_key_released(int keycode)
{
    if (!is_pressed_now(keycode)) {
        return true;
    }

    send_key_event(keycode, true);
    pump_messages();
    return !is_pressed_now(keycode);
}

bool BackgroundManagedKeyInput::wait_until_applied(uint64_t generation)
{
    std::unique_lock lock(mutex_);
    const bool applied = applied_cv_.wait_for(lock, apply_timeout, [this, generation] { return applied_generation_ >= generation; });
    if (!applied) {
        LogWarn << "Timed out waiting for background managed key state to apply" << VAR(generation) << VAR(applied_generation_);
    }
    return applied;
}

MAA_CTRL_UNIT_NS_END
