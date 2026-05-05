#include "GetCursorPosHook.h"

#include "GetCursorPosHookShared.h"

#include "MaaUtils/Logger.h"

#include <filesystem>
#include <iterator>

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

int module_anchor = 0;

}

GetCursorPosHook::~GetCursorPosHook()
{
    clear_pos();

    if (shared_state_) {
        UnmapViewOfFile(shared_state_);
        shared_state_ = nullptr;
    }
    if (mapping_) {
        CloseHandle(mapping_);
        mapping_ = nullptr;
    }
    if (process_) {
        CloseHandle(process_);
        process_ = nullptr;
    }
}

bool GetCursorPosHook::set_pos(HWND target, POINT screen_pos)
{
    if (!ensure_ready(target)) {
        return false;
    }

    InterlockedExchange(&shared_state_->x, screen_pos.x);
    InterlockedExchange(&shared_state_->y, screen_pos.y);
    InterlockedExchange(&shared_state_->active, 1);
    return true;
}

void GetCursorPosHook::clear_pos()
{
    if (!shared_state_) {
        return;
    }
    InterlockedExchange(&shared_state_->active, 0);
}

bool GetCursorPosHook::ensure_ready(HWND target)
{
    if (!target || !IsWindow(target)) {
        LogError << "GetCursorPos hook target window is invalid" << VAR(target);
        return false;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(target, &pid);
    if (!pid) {
        LogError << "GetWindowThreadProcessId failed for GetCursorPos hook" << VAR(target) << VAR(GetLastError());
        return false;
    }

    if (ready_ && pid_ == pid) {
        return true;
    }

    ready_ = false;

    if (!open_target_process(pid)) {
        return false;
    }
    if (!open_shared_state(pid)) {
        return false;
    }

    if (InterlockedCompareExchange(&shared_state_->hook_ready, 0, 0) != 1 && !inject_hook_dll()) {
        return false;
    }

    for (int i = 0; i != 100; ++i) {
        if (InterlockedCompareExchange(&shared_state_->hook_ready, 0, 0) == 1) {
            ready_ = true;
            return true;
        }
        if (InterlockedCompareExchange(&shared_state_->last_error, 0, 0) != 0) {
            break;
        }
        Sleep(10);
    }

    LogError << "GetCursorPos hook is not ready" << VAR(pid_) << VAR(shared_state_->last_error);
    return false;
}

bool GetCursorPosHook::open_target_process(DWORD pid)
{
    if (process_ && pid_ == pid) {
        DWORD exit_code = 0;
        // Only reuse the cached handle if the process is still active.
        if (GetExitCodeProcess(process_, &exit_code) && exit_code == STILL_ACTIVE) {
            return true;
        }

        // The cached handle is no longer valid (process exited or query failed).
        // Close and reset so we will reopen the process below.
        CloseHandle(process_);
        process_ = nullptr;
        pid_ = 0;
    }

    if (process_) {
        CloseHandle(process_);
        process_ = nullptr;
    }

    static constexpr DWORD kAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE;
    process_ = OpenProcess(kAccess, FALSE, pid);
    if (!process_) {
        LogError << "OpenProcess failed for GetCursorPos hook" << VAR(pid) << VAR(GetLastError());
        return false;
    }

    pid_ = pid;
    return true;
}

bool GetCursorPosHook::open_shared_state(DWORD pid)
{
    if (mapping_ && shared_state_ && pid_ == pid) {
        return true;
    }

    if (shared_state_) {
        UnmapViewOfFile(shared_state_);
        shared_state_ = nullptr;
    }
    if (mapping_) {
        CloseHandle(mapping_);
        mapping_ = nullptr;
    }

    auto mapping_name = make_mapping_name(pid);
    mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(MaaWin32GetCursorPosHook::SharedState), mapping_name.c_str());
    if (!mapping_) {
        LogError << "CreateFileMappingW failed for GetCursorPos hook" << VAR(pid) << VAR(GetLastError());
        return false;
    }

    shared_state_ = static_cast<MaaWin32GetCursorPosHook::SharedState*>(
        MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MaaWin32GetCursorPosHook::SharedState)));
    if (!shared_state_) {
        LogError << "MapViewOfFile failed for GetCursorPos hook" << VAR(pid) << VAR(GetLastError());
        CloseHandle(mapping_);
        mapping_ = nullptr;
        return false;
    }

    if (shared_state_->magic != MaaWin32GetCursorPosHook::kSharedStateMagic
        || shared_state_->version != MaaWin32GetCursorPosHook::kSharedStateVersion) {
        *shared_state_ = MaaWin32GetCursorPosHook::SharedState {};
    }

    InterlockedExchange(&shared_state_->active, 0);
    return true;
}

bool GetCursorPosHook::inject_hook_dll()
{
    auto dll_path = get_hook_dll_path();
    if (dll_path.empty()) {
        return false;
    }

    const auto bytes = (dll_path.size() + 1) * sizeof(wchar_t);
    void* remote_path = VirtualAllocEx(process_, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path) {
        LogError << "VirtualAllocEx failed for GetCursorPos hook" << VAR(pid_) << VAR(GetLastError());
        return false;
    }

    bool success = false;
    HANDLE thread = nullptr;
    do {
        if (!WriteProcessMemory(process_, remote_path, dll_path.c_str(), bytes, nullptr)) {
            LogError << "WriteProcessMemory failed for GetCursorPos hook" << VAR(pid_) << VAR(GetLastError());
            break;
        }

        auto load_library = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
        if (!load_library) {
            LogError << "GetProcAddress(LoadLibraryW) failed for GetCursorPos hook" << VAR(GetLastError());
            break;
        }

        thread = CreateRemoteThread(process_, nullptr, 0, load_library, remote_path, 0, nullptr);
        if (!thread) {
            LogError << "CreateRemoteThread failed for GetCursorPos hook" << VAR(pid_) << VAR(GetLastError());
            break;
        }

        if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) {
            LogError << "LoadLibraryW timed out for GetCursorPos hook" << VAR(pid_);
            break;
        }

        DWORD exit_code = 0;
        if (!GetExitCodeThread(thread, &exit_code) || exit_code == 0) {
            LogError << "LoadLibraryW failed for GetCursorPos hook" << VAR(pid_) << VAR(GetLastError()) << VAR(exit_code);
            break;
        }

        success = true;
    } while (false);

    if (thread) {
        CloseHandle(thread);
    }
    VirtualFreeEx(process_, remote_path, 0, MEM_RELEASE);
    return success;
}

std::wstring GetCursorPosHook::make_mapping_name(DWORD pid) const
{
    return std::wstring(MaaWin32GetCursorPosHook::kMappingNamePrefix) + std::to_wstring(pid);
}

std::wstring GetCursorPosHook::get_hook_dll_path() const
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&module_anchor),
            &module)) {
        LogError << "GetModuleHandleExW failed for GetCursorPos hook" << VAR(GetLastError());
        return {};
    }

    wchar_t module_path[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(module, module_path, static_cast<DWORD>(std::size(module_path)));
    if (len == 0 || len == std::size(module_path)) {
        LogError << "GetModuleFileNameW failed for GetCursorPos hook" << VAR(GetLastError());
        return {};
    }

    auto path = std::filesystem::path(module_path).parent_path() / MaaWin32GetCursorPosHook::kHookDllName;
    return path.wstring();
}

MAA_CTRL_UNIT_NS_END
