#pragma once

#include <cstdint>

namespace MaaWin32GetCursorPosHook
{

inline constexpr uint32_t kSharedStateMagic = 0x4D414743;
inline constexpr uint32_t kSharedStateVersion = 1;
inline constexpr wchar_t kMappingNamePrefix[] = L"Local\\MaaWin32GetCursorPosHook_";
inline constexpr wchar_t kHookDllName[] = L"MaaWin32GetCursorPosHook.dll";

struct SharedState
{
    uint32_t magic = kSharedStateMagic;
    uint32_t version = kSharedStateVersion;
    volatile long hook_ready = 0;
    volatile long active = 0;
    volatile long x = 0;
    volatile long y = 0;
    volatile long last_error = 0;
};

}
