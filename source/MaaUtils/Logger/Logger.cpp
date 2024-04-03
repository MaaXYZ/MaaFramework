#define _CRT_SECURE_NO_WARNINGS

#include "Utils/Logger.h"

#ifdef _WIN32
#include "Utils/SafeWindows.hpp"

#include <io.h>
#include <sysinfoapi.h>
#else
#include <sys/utsname.h>
#endif

#include "Utils/Codec.h"
#include "Utils/ImageIo.h"
#include "Utils/Platform.h"
#include "Utils/Uuid.h"

#pragma message("MaaUtils MAA_VERSION: " MAA_VERSION)

MAA_LOG_NS_BEGIN

std::string utf8_to_crt(std::string_view utf8_str)
{
#ifdef _WIN32
    const char* src_str = utf8_str.data();
    const int byte_len = static_cast<int>(utf8_str.length() * sizeof(char));
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, byte_len, nullptr, 0);
    const std::size_t wsz_ansi_length = static_cast<std::size_t>(len) + 1U;
    auto wsz_ansi = new wchar_t[wsz_ansi_length];
    memset(wsz_ansi, 0, sizeof(wsz_ansi[0]) * wsz_ansi_length);
    MultiByteToWideChar(CP_UTF8, 0, src_str, byte_len, wsz_ansi, len);

    len = WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, nullptr, 0, nullptr, nullptr);
    const std::size_t sz_ansi_length = static_cast<std::size_t>(len) + 1;
    auto sz_ansi = new char[sz_ansi_length];
    memset(sz_ansi, 0, sizeof(sz_ansi[0]) * sz_ansi_length);
    WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, sz_ansi, len, nullptr, nullptr);
    std::string strTemp(sz_ansi);

    delete[] wsz_ansi;
    wsz_ansi = nullptr;
    delete[] sz_ansi;
    sz_ansi = nullptr;

    return strTemp;
#else
    return std::string(utf8_str);
#endif
}

constexpr separator separator::none("");
constexpr separator separator::space(" ");
constexpr separator separator::tab("\t");
constexpr separator separator::newline("\n");
constexpr separator separator::comma(",");

static constexpr std::string_view kSplitLine = "-----------------------------";

std::string LogStream::stdout_string()
{
    std::string color;

    switch (lv_) {
    case level::fatal:
    case level::error:
        color = "\033[31m";
        break;
    case level::warn:
        color = "\033[33m";
        break;
    case level::info:
        color = "\033[32m";
        break;
    case level::debug:
    case level::trace:
        break;
    }

    return color + utf8_to_crt(buffer_.str()) + "\033[0m";
}

std::string_view LogStream::level_str()
{
    switch (lv_) {
    case level::fatal:
        return "FTL";
    case level::error:
        return "ERR";
    case level::warn:
        return "WRN";
    case level::info:
        return "INF";
    case level::debug:
        return "DBG";
    case level::trace:
        return "TRC";
    }
    return "NoLV";
}

Logger& Logger::get_instance()
{
    static Logger unique_instance;
    return unique_instance;
}

void Logger::start_logging(std::filesystem::path dir)
{
    log_dir_ = std::move(dir);
    log_path_ = log_dir_ / kLogFilename;
    dumps_dir_ = log_dir_ / kDumpsDirname;
    reinit();
}

void Logger::set_stdout_level(MaaLoggingLevel level)
{
    stdout_level_ = level;
}

void Logger::flush()
{
    internal_dbg() << kSplitLine;
    internal_dbg() << "Flush log";
    internal_dbg() << kSplitLine;

    bool rotated = rotate();
    open();

    if (rotated) {
        log_proc_info();
    }
}

void Logger::reinit()
{
    rotate();
    open();
    log_proc_info();
}

bool Logger::rotate()
{
    if (log_path_.empty() || !std::filesystem::exists(log_path_)) {
        return false;
    }

    std::unique_lock<std::mutex> m_trace_lock(trace_mutex_);
    if (ofs_.is_open()) {
        ofs_.close();
    }

    constexpr uintmax_t MaxLogSize = 16ULL * 1024 * 1024;
    const uintmax_t log_size = std::filesystem::file_size(log_path_);
    if (log_size < MaxLogSize) {
        return false;
    }

    std::error_code ec;

    const std::filesystem::path bak_path = log_dir_ / kLogbakFilename;
    std::filesystem::rename(log_path_, bak_path, ec);

    const std::filesystem::path dumps_bak_path = log_dir_ / kDumpsbakDirname;
    if (std::filesystem::exists(dumps_bak_path)) {
        std::filesystem::remove_all(dumps_bak_path, ec);
    }
    if (std::filesystem::exists(dumps_dir_)) {
        std::filesystem::rename(dumps_dir_, dumps_bak_path, ec);
    }

    return true;
}

void Logger::open()
{
    if (log_path_.empty()) {
        return;
    }

    std::filesystem::create_directories(log_dir_);

    std::unique_lock<std::mutex> m_trace_lock(trace_mutex_);
    if (ofs_.is_open()) {
        ofs_.close();
    }

#ifdef _WIN32

    // https://stackoverflow.com/questions/55513974/controlling-inheritability-of-file-handles-created-by-c-stdfstream-in-window
    std::string str_log_path = path_to_crt_string(log_path_);
    FILE* file_ptr = fopen(str_log_path.c_str(), "a");
    SetHandleInformation((HANDLE)_get_osfhandle(_fileno(file_ptr)), HANDLE_FLAG_INHERIT, 0);
    ofs_ = std::ofstream(file_ptr);

#else

    ofs_ = std::ofstream(log_path_, std::ios::out | std::ios::app);

#endif
}

void Logger::close()
{
    internal_dbg() << kSplitLine;
    internal_dbg() << "Close log";
    internal_dbg() << kSplitLine;

    std::unique_lock<std::mutex> m_trace_lock(trace_mutex_);
    if (ofs_.is_open()) {
        ofs_.close();
    }
}

static std::string sys_info()
{
#ifdef _WIN32
    SYSTEM_INFO sys {};
    GetNativeSystemInfo(&sys);

    // https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
    std::string arch;
    switch (sys.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        arch = "x64";
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        arch = "ARM64";
        break;
    default:
        arch = "arch" + std::to_string(sys.wProcessorArchitecture);
        break;
    }
    return std::format("{} {}", "Windows", arch);
#else
    utsname uts {};
    uname(&uts);
    return std::format("{} {}", uts.sysname, uts.machine);
#endif
}

void Logger::log_proc_info()
{
    internal_dbg() << kSplitLine;
    internal_dbg() << "MAA Process Start";
    internal_dbg() << "Version" << MAA_VERSION;
    internal_dbg() << "Built at" << __DATE__ << __TIME__;
    internal_dbg() << sys_info();
    internal_dbg() << "Working" << std::filesystem::current_path();
    internal_dbg() << "Logging" << log_path_;
    internal_dbg() << kSplitLine;
}

LogStream Logger::internal_dbg()
{
    return debug("Logger");
}

std::string StringConverter::operator()(const std::filesystem::path& path) const
{
    return path_to_utf8_string(path);
}

std::string StringConverter::operator()(const std::wstring& wstr) const
{
    return from_u16(wstr);
}

std::string StringConverter::operator()(const cv::Mat& image) const
{
    if (dumps_dir_.empty()) {
        return "Not logging";
    }
    if (image.empty()) {
        return "Empty image";
    }

    std::string filename = std::format("{}-{}.png", format_now_for_filename(), make_uuid());
    auto filepath = dumps_dir_ / path(filename);
    bool ret = MAA_NS::imwrite(filepath, image);
    if (!ret) {
        return "Failed to write image";
    }
    return this->operator()(filepath);
}

MAA_LOG_NS_END
