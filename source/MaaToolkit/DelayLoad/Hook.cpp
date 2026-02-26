#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <delayimp.h>
#include <string.h>

static HMODULE fw_dll = NULL;

static FARPROC WINAPI load_hook(unsigned int event, DelayLoadInfo* info)
{
    if (event == dliNotePreGetProcAddress) {
        if (!fw_dll) {
            return NULL;
        }
        return GetProcAddress(fw_dll, info->dlp.szProcName);
    }
    if (event == dliStartProcessing) {
        fw_dll = GetModuleHandleA("MaaFramework.dll");
        if (!fw_dll) {
            fw_dll = GetModuleHandleA("MaaAgentServer.dll");
        }
        return NULL;
    }
    return NULL;
}

decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = load_hook;

#endif
