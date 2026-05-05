#pragma once

#include "MaaUtils/SafeWindows.hpp"

#include <string>

namespace MaaWin32GetCursorPosHook
{
struct SharedState;
}

MAA_CTRL_UNIT_NS_BEGIN

class GetCursorPosHook
{
public:
    GetCursorPosHook() = default;
    ~GetCursorPosHook();

    bool set_pos(HWND target, POINT screen_pos);
    void clear_pos();

private:
    bool ensure_ready(HWND target);
    bool open_target_process(DWORD pid);
    bool open_shared_state(DWORD pid);
    bool inject_hook_dll();

    std::wstring make_mapping_name(DWORD pid) const;
    std::wstring get_hook_dll_path() const;

    DWORD pid_ = 0;
    HANDLE process_ = nullptr;
    HANDLE mapping_ = nullptr;
    MaaWin32GetCursorPosHook::SharedState* shared_state_ = nullptr;
    bool ready_ = false;
};

MAA_CTRL_UNIT_NS_END
