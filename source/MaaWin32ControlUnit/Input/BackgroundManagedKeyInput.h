#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class BackgroundManagedKeyInput
{
public:
    explicit BackgroundManagedKeyInput(HWND hwnd);
    ~BackgroundManagedKeyInput();

public:
    bool set_managed_keys(const std::vector<int>& keycodes);
    bool is_managed_key(int keycode) const;
    bool is_key_pressed(int keycode) const;
    bool key_down(int keycode);
    bool key_up(int keycode);
    bool inactive();

private:
    struct Snapshot
    {
        std::vector<int> keys;
        std::unordered_set<int> desired_pressed_keys;
        std::vector<int> release_keys;
        uint64_t generation = 0;
    };

private:
    static std::unordered_set<int> normalize_keycodes(const std::vector<int>& keycodes);
    static bool is_valid_keycode(int keycode);
    static int hotkey_id(int keycode);
    static bool is_pressed_now(int keycode);
    static void send_key_event(int keycode, bool key_up);
    static void pump_messages();
    static void send_f13_nudge();

    void send_activation_hint() const;
    Snapshot snapshot_locked() const;
    void guard_loop();
    bool correct_snapshot(const Snapshot& snapshot);
    bool ensure_key_state(int keycode, bool desired_pressed);
    bool ensure_key_pressed(int keycode);
    bool ensure_key_released(int keycode);
    bool wait_until_applied(uint64_t generation);

private:
    HWND hwnd_ = nullptr;

    mutable std::mutex mutex_;
    std::condition_variable guard_cv_;
    std::condition_variable applied_cv_;
    std::unordered_set<int> managed_keys_;
    std::unordered_set<int> desired_pressed_keys_;
    std::unordered_set<int> release_keys_;
    uint64_t desired_generation_ = 0;
    uint64_t applied_generation_ = 0;
    bool stop_thread_ = false;
    std::thread guard_thread_;
};

MAA_CTRL_UNIT_NS_END
