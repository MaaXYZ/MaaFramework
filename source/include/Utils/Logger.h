#pragma once

#include <source_location>

#include "LoggerUtils.h"
#include "ScopeLeave.hpp"

MAA_LOG_NS_BEGIN

class MAA_UTILS_API Logger
{
public:
    static constexpr std::string_view kLogFilename = "maa.log";
    static constexpr std::string_view kLogbakFilename = "maa.bak.log";
    static constexpr std::string_view kDumpsDirname = "dumps";
    static constexpr std::string_view kDumpsbakDirname = "dumps.bak";

public:
    static Logger& get_instance();

    ~Logger() { close(); }

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    auto fatal(const std::source_location& loc = std::source_location::current())
    {
        return stream(level::fatal, loc);
    }

    auto error(const std::source_location& loc = std::source_location::current())
    {
        return stream(level::error, loc);
    }

    auto warn(const std::source_location& loc = std::source_location::current())
    {
        return stream(level::warn, loc);
    }

    auto info(const std::source_location& loc = std::source_location::current())
    {
        return stream(level::info, loc);
    }

    auto debug(const std::source_location& loc = std::source_location::current())
    {
        return stream(level::debug, loc);
    }

    auto trace(const std::source_location& loc = std::source_location::current())
    {
        return stream(level::trace, loc);
    }

    void start_logging(std::filesystem::path dir);
    void set_stdout_level(MaaLoggingLevel level);
    void flush();

private:
    template <typename... args_t>
    LogStream stream(level lv, args_t&&... args)
    {
        bool std_out = static_cast<int>(lv) <= stdout_level_;
        return LogStream(
            trace_mutex_,
            ofs_,
            lv,
            std_out,
            dumps_dir_,
            std::forward<args_t>(args)...);
    }

private:
    Logger() = default;

    void reinit();
    bool rotate();
    void open();
    void close();
    void log_proc_info();

    LogStream internal_dbg();

private:
    std::filesystem::path log_dir_;
    std::filesystem::path log_path_;
    std::filesystem::path dumps_dir_;

#ifdef MAA_DEBUG
    MaaLoggingLevel stdout_level_ = MaaLoggingLevel_All;
#else
    MaaLoggingLevel stdout_level_ = MaaLoggingLevel_Error;
#endif
    std::ofstream ofs_;
    std::mutex trace_mutex_;
};

class LogScopeEnterHelper
{
public:
    explicit LogScopeEnterHelper(const std::source_location& loc = std::source_location::current())
        : stream_(Logger::get_instance().debug(loc))
    {
    }

    ~LogScopeEnterHelper() { stream_ << "| enter"; }

    LogStream& operator()() { return stream_; }

private:
    LogStream stream_;
};

class LogScopeLeaveHelper
{
public:
    explicit LogScopeLeaveHelper(std::source_location&& loc = std::source_location::current())
        : loc_(loc)
    {
    }

    ~LogScopeLeaveHelper()
    {
        Logger::get_instance().trace(loc_) << "| leave," << duration_since(start_);
    }

private:
    std::source_location loc_;
    std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
};

MAA_LOG_NS_END

#define LogFatal MAA_LOG_NS::Logger::get_instance().fatal()
#define LogError MAA_LOG_NS::Logger::get_instance().error()
#define LogWarn MAA_LOG_NS::Logger::get_instance().warn()
#define LogInfo MAA_LOG_NS::Logger::get_instance().info()
#define LogDebug MAA_LOG_NS::Logger::get_instance().debug()
#define LogTrace MAA_LOG_NS::Logger::get_instance().trace()

#define LogFunc                                         \
    MAA_LOG_NS::LogScopeLeaveHelper ScopeHelperVarName; \
    MAA_LOG_NS::LogScopeEnterHelper()()

#define VAR_RAW(x) "[" << #x << "=" << (x) << "] "
#define VAR(x) MAA_LOG_NS::separator::none << VAR_RAW(x) << MAA_LOG_NS::separator::space
#define VAR_VOIDP_RAW(x) "[" << #x << "=" << reinterpret_cast<void*>(x) << "] "
#define VAR_VOIDP(x) MAA_LOG_NS::separator::none << VAR_VOIDP_RAW(x) << MAA_LOG_NS::separator::space
