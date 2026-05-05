#include "GetCursorPosHookShared.h"

#include <windows.h>
#include <tlhelp32.h>

#include <iterator>

namespace
{

using GetCursorPosFn = BOOL(WINAPI*)(LPPOINT);

enum HookError : long
{
    HookError_None = 0,
    HookError_User32Missing = 1,
    HookError_GetCursorPosMissing = 2,
    HookError_OpenMappingFailed = 3,
    HookError_MapViewFailed = 4,
    HookError_ModuleSnapshotFailed = 5,
    HookError_NoImportPatched = 6,
};

GetCursorPosFn g_original_get_cursor_pos = nullptr;
MaaWin32GetCursorPosHook::SharedState* g_state = nullptr;

bool ascii_iequals(const char* lhs, const char* rhs)
{
    while (*lhs && *rhs) {
        char a = *lhs;
        char b = *rhs;
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
        ++lhs;
        ++rhs;
    }
    return *lhs == '\0' && *rhs == '\0';
}

void append_uint(wchar_t* buffer, size_t capacity, DWORD value)
{
    size_t len = 0;
    while (len + 1 < capacity && buffer[len] != L'\0') {
        ++len;
    }

    wchar_t digits[16] = {};
    size_t count = 0;
    do {
        digits[count++] = static_cast<wchar_t>(L'0' + (value % 10));
        value /= 10;
    } while (value && count < std::size(digits));

    while (count != 0 && len + 1 < capacity) {
        buffer[len++] = digits[--count];
    }
    buffer[len] = L'\0';
}

BOOL WINAPI hooked_get_cursor_pos(LPPOINT point)
{
    if (point && g_state && g_state->magic == MaaWin32GetCursorPosHook::kSharedStateMagic
        && g_state->version == MaaWin32GetCursorPosHook::kSharedStateVersion
        && InterlockedCompareExchange(&g_state->active, 0, 0) != 0) {
        point->x = InterlockedCompareExchange(&g_state->x, 0, 0);
        point->y = InterlockedCompareExchange(&g_state->y, 0, 0);
        return TRUE;
    }

    return g_original_get_cursor_pos(point);
}

bool patch_thunk(IMAGE_THUNK_DATA* thunk)
{
    DWORD old_protect = 0;
    if (!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), PAGE_READWRITE, &old_protect)) {
        return false;
    }

    thunk->u1.Function = reinterpret_cast<ULONG_PTR>(&hooked_get_cursor_pos);
    FlushInstructionCache(GetCurrentProcess(), &thunk->u1.Function, sizeof(thunk->u1.Function));

    DWORD unused = 0;
    VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), old_protect, &unused);
    return true;
}

bool patch_module_import(HMODULE module, size_t& patched_count)
{
    auto* base = reinterpret_cast<unsigned char*>(module);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return true;
    }

    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return true;
    }

    const auto& import_dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!import_dir.VirtualAddress || !import_dir.Size) {
        return true;
    }

    auto* imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + import_dir.VirtualAddress);
    for (; imports->Name; ++imports) {
        auto* dll_name = reinterpret_cast<const char*>(base + imports->Name);
        if (!ascii_iequals(dll_name, "user32.dll")) {
            continue;
        }

        auto* thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->FirstThunk);
        auto* original_thunk = imports->OriginalFirstThunk ? reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->OriginalFirstThunk) : nullptr;

        for (; thunk->u1.Function; ++thunk) {
            bool is_get_cursor_pos = reinterpret_cast<void*>(thunk->u1.Function) == reinterpret_cast<void*>(g_original_get_cursor_pos);
            if (!is_get_cursor_pos && original_thunk && !IMAGE_SNAP_BY_ORDINAL(original_thunk->u1.Ordinal)) {
                auto* import_by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + original_thunk->u1.AddressOfData);
                is_get_cursor_pos = ascii_iequals(reinterpret_cast<const char*>(import_by_name->Name), "GetCursorPos");
            }

            if (is_get_cursor_pos && patch_thunk(thunk)) {
                ++patched_count;
            }

            if (original_thunk) {
                ++original_thunk;
            }
        }
    }

    return true;
}

bool install_iat_hook()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    size_t patched_count = 0;
    MODULEENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (Module32FirstW(snapshot, &entry)) {
        do {
            patch_module_import(entry.hModule, patched_count);
        } while (Module32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return patched_count != 0;
}

bool open_shared_state()
{
    wchar_t mapping_name[128] = {};
    lstrcpynW(mapping_name, MaaWin32GetCursorPosHook::kMappingNamePrefix, static_cast<int>(std::size(mapping_name)));
    append_uint(mapping_name, std::size(mapping_name), GetCurrentProcessId());

    HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!mapping) {
        return false;
    }

    g_state = static_cast<MaaWin32GetCursorPosHook::SharedState*>(MapViewOfFile(
        mapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(MaaWin32GetCursorPosHook::SharedState)));
    CloseHandle(mapping);
    return g_state != nullptr && g_state->magic == MaaWin32GetCursorPosHook::kSharedStateMagic
           && g_state->version == MaaWin32GetCursorPosHook::kSharedStateVersion;
}

void set_hook_error(HookError error)
{
    if (g_state) {
        InterlockedExchange(&g_state->last_error, error);
    }
}

void initialize()
{
    if (!open_shared_state()) {
        return;
    }

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) {
        set_hook_error(HookError_User32Missing);
        return;
    }

    g_original_get_cursor_pos = reinterpret_cast<GetCursorPosFn>(GetProcAddress(user32, "GetCursorPos"));
    if (!g_original_get_cursor_pos) {
        set_hook_error(HookError_GetCursorPosMissing);
        return;
    }

    if (!install_iat_hook()) {
        set_hook_error(HookError_NoImportPatched);
        return;
    }

    InterlockedExchange(&g_state->last_error, HookError_None);
    InterlockedExchange(&g_state->hook_ready, 1);
}

DWORD WINAPI initialize_thread(LPVOID)
{
    initialize();
    return 0;
}

}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, initialize_thread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
