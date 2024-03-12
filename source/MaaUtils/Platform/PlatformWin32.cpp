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

std::string get_ansi_short_path(const std::filesystem::path& path)
{
    wchar_t short_path[MAX_PATH] = { 0 };
    auto osstr = path.native();
    string_replace_all_(osstr, L"\\", L"/");
    auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
    if (shortlen == 0) {
        return {};
    }
    BOOL failed = FALSE;
    auto ansilen =
        WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
    if (failed) {
        return {};
    }
    std::string result(ansilen, 0);
    WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, result.data(), ansilen, nullptr, nullptr);
    return result;
}

std::string path_to_crt_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    size_t mbsize = 0;
    auto osstr = path.native();
    string_replace_all_(osstr, L"\\", L"/");
    auto err = wcstombs_s(&mbsize, nullptr, 0, osstr.c_str(), osstr.size());
    if (err != 0) {
        // cannot convert (CRT is not using UTF-8), fallback to short path name in ACP
        return get_ansi_short_path(path);
    }
    std::string result(mbsize, 0);
    err = wcstombs_s(&mbsize, result.data(), mbsize, osstr.c_str(), osstr.size());
    if (err != 0) {
        return {};
    }
    return result.substr(0, mbsize - 1);
}

std::string path_to_ansi_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    BOOL failed = FALSE;
    auto osstr = path.native();
    string_replace_all_(osstr, L"\\", L"/");
    auto ansilen = WideCharToMultiByte(
        CP_ACP,
        0,
        osstr.c_str(),
        (int)osstr.size(),
        nullptr,
        0,
        nullptr,
        &failed);
    if (failed) {
        // contains character that cannot be converted, fallback to short path name in ACP
        return get_ansi_short_path(path);
    }
    std::string result(ansilen, 0);
    WideCharToMultiByte(
        CP_ACP,
        0,
        osstr.c_str(),
        (int)osstr.size(),
        result.data(),
        ansilen,
        nullptr,
        &failed);
    return result;
}

os_string to_osstring(std::string_view utf8_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), nullptr, 0);
    os_string result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), result.data(), len);
    return result;
}

std::string from_osstring(os_string_view os_str)
{
    int len = WideCharToMultiByte(
        CP_UTF8,
        0,
        os_str.data(),
        (int)os_str.size(),
        nullptr,
        0,
        nullptr,
        nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(
        CP_UTF8,
        0,
        os_str.data(),
        (int)os_str.size(),
        result.data(),
        len,
        nullptr,
        nullptr);
    return result;
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
        if (process == nullptr) {
            auto error = GetLastError();
            LogWarn << "Failed to OpenProcess" << VAR(error) << VAR(pid);
            continue;
        }

        HMODULE mod = nullptr;
        DWORD mod_read = 0;
        if (!EnumProcessModules(process, &mod, sizeof(mod), &mod_read)) {
            auto error = GetLastError();
            LogWarn << "Failed to EnumProcessModules" << VAR(error) << VAR(pid);
            CloseHandle(process);
            continue;
        }

        memset(name_buff, 0, sizeof(name_buff));

        GetModuleBaseNameW(process, mod, name_buff, sizeof(name_buff) / sizeof(WCHAR));
        CloseHandle(process);

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
    if (process == nullptr) {
        auto error = GetLastError();
        LogError << "Failed to OpenProcess" << VAR(error) << VAR(pid);
        return std::nullopt;
    }

    WCHAR filename[MAX_PATH] = { 0 };
    if (!GetModuleFileNameExW(process, NULL, filename, MAX_PATH)) {
        auto error = GetLastError();
        LogError << "Failed to GetModuleFileNameEx" << VAR(error) << VAR(pid) << VAR_VOIDP(process);
        return std::nullopt;
    }

    CloseHandle(process);
    return filename;
}

MAA_NS_END

#endif