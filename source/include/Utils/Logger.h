#pragma once

#include <source_location>

#include "LoggerUtils.h"

MAA_LOG_NS_BEGIN

class MAA_UTILS_API Logger
{
public:
    static constexpr std::string_view kLogFilename = "maa.log";
    static constexpr std::string_view kLogbakFilename = "maa.bak.log";

public:
    static Logger& get_instance();
    ~Logger() { close(); }

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    template <typename... args_t>
    auto fatal(args_t&&... args)
    {
        return stream(level::fatal, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto error(args_t&&... args)
    {
        return stream(level::error, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto warn(args_t&&... args)
    {
        return stream(level::warn, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto info(args_t&&... args)
    {
        return stream(level::info, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto debug(args_t&&... args)
    {
        return stream(level::debug, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto trace(args_t&&... args)
    {
        return stream(level::trace, std::forward<args_t>(args)...);
    }

    void start_logging(std::filesystem::path dir);
    void set_stdout_level(MaaLoggingLevel level);
    void flush();

private:
    template <typename... args_t>
    LogStream stream(level lv, args_t&&... args)
    {
        bool std_out = static_cast<int>(lv) <= stdout_level_;
        return LogStream(trace_mutex_, ofs_, lv, std_out, std::forward<args_t>(args)...);
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
#ifdef MAA_DEBUG
    MaaLoggingLevel stdout_level_ = MaaLoggingLevel_All;
#else
    MaaLoggingLevel stdout_level_ = MaaLoggingLevel_Error;
#endif
    std::ofstream ofs_;
    std::mutex trace_mutex_;
};

class ScopeEnterHelper
{
public:
    template <typename... args_t>
    ScopeEnterHelper(args_t&&... args) : stream_(Logger::get_instance().debug(std::forward<args_t>(args)...))
    {}
    ~ScopeEnterHelper() { stream_ << "| enter"; }

    LogStream& operator()() { return stream_; }

private:
    LogStream stream_;
};

template <typename... args_t>
class ScopeLeaveHelper
{
public:
    ScopeLeaveHelper(args_t&&... args) : args_(std::forward<args_t>(args)...) {}
    ~ScopeLeaveHelper()
    {
        std::apply([](auto&&... args) { return Logger::get_instance().debug(std::forward<decltype(args)>(args)...); },
                   std::move(args_))
            << "| leave," << duration_since(start_);
    }

private:
    std::tuple<args_t...> args_;
    std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
};

inline constexpr std::string_view pertty_file(std::string_view file)
{
    size_t pos = file.find_last_of(std::filesystem::path::preferred_separator);
    return file.substr(pos + 1, file.size());
}

MAA_LOG_NS_END

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define MAA_FILE MAA_LOG_NS::pertty_file(__FILE__)
#define MAA_LINE std::string_view("L" LINE_STRING)
#ifdef _MSC_VER
#define MAA_FUNCTION std::string_view(__FUNCTION__)
#else
#define MAA_FUNCTION std::string_view(__PRETTY_FUNCTION__)
#endif
#define LOG_ARGS MAA_FILE, MAA_LINE, MAA_FUNCTION

#define LogFatal MAA_LOG_NS::Logger::get_instance().fatal(LOG_ARGS)
#define LogError MAA_LOG_NS::Logger::get_instance().error(LOG_ARGS)
#define LogWarn MAA_LOG_NS::Logger::get_instance().warn(LOG_ARGS)
#define LogInfo MAA_LOG_NS::Logger::get_instance().info(LOG_ARGS)
#define LogDebug MAA_LOG_NS::Logger::get_instance().debug(LOG_ARGS)
#define LogTrace MAA_LOG_NS::Logger::get_instance().trace(LOG_ARGS)

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)
#define LogScopeHeplerName _CatVarNameWithLine(log_scope_)

#define LogFunc                                                \
    MAA_LOG_NS::ScopeLeaveHelper LogScopeHeplerName(LOG_ARGS); \
    MAA_LOG_NS::ScopeEnterHelper(LOG_ARGS)()

#define VAR_RAW(x) "[" << #x << "=" << (x) << "] "
#define VAR(x) MAA_LOG_NS::separator::none << VAR_RAW(x) << MAA_LOG_NS::separator::space
#define VAR_VOIDP_RAW(x) "[" << #x << "=" << ((void*)x) << "] "
#define VAR_VOIDP(x) MAA_LOG_NS::separator::none << VAR_VOIDP_RAW(x) << MAA_LOG_NS::separator::space
