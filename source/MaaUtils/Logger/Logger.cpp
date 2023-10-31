#define _CRT_SECURE_NO_WARNINGS

#include "Utils/Logger.h"

#include "Utils/Platform.h"

#ifdef _WIN32
#include <io.h>
#endif

#pragma message("MaaUtils MAA_VERSION: " MAA_VERSION)

MAA_LOG_NS_BEGIN

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

    constexpr uintmax_t MaxLogSize = 4ULL * 1024 * 1024;
    const uintmax_t log_size = std::filesystem::file_size(log_path_);
    if (log_size < MaxLogSize) {
        return false;
    }

    const std::filesystem::path bak_path = log_dir_ / kLogbakFilename;
    try {
        std::filesystem::rename(log_path_, bak_path);
    }
    catch (...) {
        return false;
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

void Logger::log_proc_info()
{
    internal_dbg() << kSplitLine;
    internal_dbg() << "MAA Process Start";
    internal_dbg() << "Version" << MAA_VERSION;
    internal_dbg() << "Built at" << __DATE__ << __TIME__;
    internal_dbg() << "Log Path" << log_path_;
    internal_dbg() << kSplitLine;
}

LogStream Logger::internal_dbg()
{
    return debug("Logger");
}

MAA_LOG_NS_END
