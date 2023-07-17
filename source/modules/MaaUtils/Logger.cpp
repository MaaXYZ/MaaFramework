#include "MaaUtils/Logger.hpp"

#pragma message("MAA_VERSION: " MAA_VERSION)

MAA_NS_BEGIN

Logger& Logger::get_instance()
{
    static Logger unique_instance;
    return unique_instance;
}

void Logger::start_logging(std::filesystem::path dir)
{
    log_path_ = std::move(dir) / "maa.log";
    reinit();
}

void Logger::flush()
{
    internal_trace() << "-----------------------------";
    internal_trace() << "Flush log";
    internal_trace() << "-----------------------------";

    rotate();
    open();
}

void Logger::reinit()
{
    rotate();
    open();
    log_start();
}

void Logger::rotate()
{
    if (log_path_.empty() || !std::filesystem::exists(log_path_)) {
        return;
    }

    std::unique_lock<std::mutex> m_trace_lock(trace_mutex_);
    if (ofs_.is_open()) {
        ofs_.close();
    }

    constexpr uintmax_t MaxLogSize = 4ULL * 1024 * 1024;
    const uintmax_t log_size = std::filesystem::file_size(log_path_);
    if (log_size < MaxLogSize) {
        return;
    }

    const std::filesystem::path bak_path = log_path_.parent_path() / "maa.bak.log";
    try {
        std::filesystem::rename(log_path_, bak_path);
    }
    catch (...) {
    }
}

void Logger::open()
{
    if (log_path_.empty()) {
        return;
    }

    std::filesystem::create_directories(log_path_.parent_path());

    std::unique_lock<std::mutex> m_trace_lock(trace_mutex_);
    if (ofs_.is_open()) {
        ofs_.close();
    }
    ofs_.open(log_path_, std::ios::out | std::ios::app);
}

void Logger::log_start()
{
    internal_trace() << "-----------------------------";
    internal_trace() << "MAA Process Start";
    internal_trace() << "Version" << MAA_VERSION;
    internal_trace() << "Built at" << __DATE__ << __TIME__;
    internal_trace() << "Log Path" << log_path_;
    internal_trace() << "-----------------------------";
}

Logger::LogStream Logger::internal_trace()
{
    return trace("Logger");
}

MAA_NS_END
