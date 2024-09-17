#ifdef _WIN32

#include "Utils/SafeWindows.hpp"

#include <Psapi.h>
#include <mbctype.h>

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

MAA_NS_BEGIN

std::string path_to_utf8_string(const std::filesystem::path& path)
{
    auto osstr = path.native();
    string_replace_all_(osstr, L"\\", L"/");
    return from_osstring(osstr);
}

os_string to_osstring(std::string_view utf8_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), nullptr, 0);
    os_string result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), result.data(), len);
    return result;
}

std::string wide_to_multi(os_string_view os_str, UINT code)
{
    int len = WideCharToMultiByte(code, 0, os_str.data(), (int)os_str.size(), nullptr, 0, nullptr, nullptr);

    std::string result(len, 0);

    WideCharToMultiByte(code, 0, os_str.data(), (int)os_str.size(), result.data(), len, nullptr, nullptr);

    return result;
}

std::string from_osstring(os_string_view os_str)
{
    return wide_to_multi(os_str, CP_UTF8);
}

std::string utf8_to_crt(std::string_view utf8_str)
{
    return wide_to_multi(to_osstring(utf8_str), CP_ACP);
}

// 转义参考:
// https://ipvb.gitee.io/windows/2019/07/21/CmdlineReEscape/
// https://github.com/microsoft/terminal/pull/1815

os_string escape_one(os_string_view arg)
{
    if (arg.empty()) {
        return os_string(2, '"');
    }

    bool space = false;
    auto len = arg.size();

    for (auto ch : arg) {
        switch (ch) {
        case '"':
        case '\\':
            len++;
            break;
        case ' ':
        case '\t':
            space = true;
            break;
        }
    }

    if (space) {
        len += 2;
    }

    if (len == arg.size()) {
        return os_string(arg);
    }

    os_string buf;
    buf.reserve(len);

    if (space) {
        buf.push_back('"');
    }

    int slash = 0;

    for (auto ch : arg) {
        switch (ch) {
        case '\\':
            slash++;
            buf.push_back('\\');
            break;
        case '"':
            buf.append(os_string(slash + 1, '\\'));
            buf.push_back('"');
            break;
        default:
            slash = 0;
            buf.push_back(ch);
        }
    }
    if (space) {
        buf.append(os_string(slash, '\\'));
        buf.push_back('"');
    }
    return buf;
}

os_string args_to_cmd(const std::vector<os_string>& args)
{
    if (args.size() == 0) {
        return os_string {};
    }

    os_string res = escape_one(args[0]);
    for (int i = 1; i < args.size(); i++) {
        res.push_back(' ');
        res.append(escape_one(args[i]));
    }
    return res;
}

os_string args_to_cmd(const std::vector<os_string_view>& args)
{
    if (args.size() == 0) {
        return os_string {};
    }

    os_string res = escape_one(args[0]);
    for (int i = 1; i < args.size(); i++) {
        res.push_back(' ');
        res.append(escape_one(args[i]));
    }
    return res;
}

std::vector<os_string> cmd_to_args(const os_string& cmd)
{
    int argc = 0;
    std::vector<os_string> res;

    auto result = CommandLineToArgvW(cmd.c_str(), &argc);

    for (int i = 0; i < argc; i++) {
        res.emplace_back(result[i]);
    }

    LocalFree(result);

    return res;
}

std::set<ProcessInfo> list_processes()
{
    // https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-processes

    constexpr size_t kMaxProcesses = 16 * 1024;

    auto all_pids = std::make_unique<DWORD[]>(kMaxProcesses);
    DWORD pid_read = 0;

    if (!EnumProcesses(all_pids.get(), sizeof(DWORD) * kMaxProcesses, &pid_read)) {
        auto error = GetLastError();
        LogError << "Failed to EnumProcesses" << VAR(error);
        return {};
    }
    DWORD size = pid_read / sizeof(DWORD);
    LogDebug << "Process size:" << size;

    WCHAR name_buff[MAX_PATH] = { 0 };

    std::set<ProcessInfo> result;

    for (DWORD i = 0; i < size; ++i) {
        DWORD pid = *(all_pids.get() + i);
        if (pid == 0) {
            continue;
        }

        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        OnScopeLeave([&]() { CloseHandle(process); });

        if (process == nullptr) {
            auto error = GetLastError();
            LogDebug << "Failed to OpenProcess" << VAR(error) << VAR(pid);
            continue;
        }

        HMODULE mod = nullptr;
        DWORD mod_read = 0;
        if (!EnumProcessModules(process, &mod, sizeof(mod), &mod_read)) {
            auto error = GetLastError();
            LogWarn << "Failed to EnumProcessModules" << VAR(error) << VAR(pid);
            continue;
        }

        memset(name_buff, 0, sizeof(name_buff));

        GetModuleBaseNameW(process, mod, name_buff, sizeof(name_buff) / sizeof(WCHAR));

        result.emplace(pid, from_osstring(name_buff));
    }

#ifdef MAA_DEBUG
    LogInfo << "Process list:" << result;
#endif

    return result;
}

std::optional<std::filesystem::path> get_process_path(os_pid pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    OnScopeLeave([&]() { CloseHandle(process); });

    if (process == nullptr) {
        auto error = GetLastError();
        LogDebug << "Failed to OpenProcess" << VAR(error) << VAR(pid);
        return std::nullopt;
    }

    WCHAR filename[MAX_PATH] = { 0 };
    if (!GetModuleFileNameExW(process, NULL, filename, MAX_PATH)) {
        auto error = GetLastError();
        LogError << "Failed to GetModuleFileNameEx" << VAR(error) << VAR(pid) << VAR_VOIDP(process);
        return std::nullopt;
    }

    return filename;
}

MAA_NS_END

#endif
