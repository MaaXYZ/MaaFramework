#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <thread>
#include <tuple>
#include <type_traits>

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaFramework/MaaPort.h"

#include "Format.hpp"
#include "Locale.hpp"
#include "Platform.h"
#include "Ranges.hpp"
#include "Time.hpp"

#define MAA_LOG_NS MAA_NS::LogNS
#define MAA_LOG_NS_BEGIN \
    namespace MAA_LOG_NS \
    {
#define MAA_LOG_NS_END }

MAA_LOG_NS_BEGIN

#ifdef __GNUC__
std::ostream& operator<<(std::ostream& os, const std::chrono::milliseconds& ms);
#endif
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& v);

template <typename T>
concept has_output_operator = requires { std::declval<std::ostream&>() << std::declval<T>(); };

enum class level
{
    fatal = MaaLoggingLevel_Fatal,
    error = MaaLoggingLevel_Error,
    warn = MaaLoggingLevel_Warn,
    info = MaaLoggingLevel_Info,
    debug = MaaLoggingLevel_Debug,
    trace = MaaLoggingLevel_Trace,
};

struct MAA_UTILS_API separator
{
    constexpr separator(std::string_view s) noexcept : str(s) {}

    static const separator none;
    static const separator space;
    static const separator tab;
    static const separator newline;
    static const separator comma;

    std::string_view str;
};

struct StringConverter
{
    template <typename T>
    static constexpr bool is_convertible = std::same_as<std::filesystem::path, std::decay_t<T>> ||
                                           std::same_as<std::wstring, std::decay_t<T>> || has_output_operator<T>;

    template <typename T>
    std::string operator()(T&& value) const
    {
        if constexpr (std::same_as<std::filesystem::path, std::decay_t<T>>) {
            return path_to_utf8_string(std::forward<T>(value));
        }
        else if constexpr (std::same_as<std::wstring, std::decay_t<T>>) {
            return from_u16(std::forward<T>(value));
        }
        else if constexpr (has_output_operator<T>) {
            return to_stringstream(std::forward<T>(value));
        }
        else {
            return json::serialize<true>(std::forward<T>(value), *this).to_string();
        }
    }

    template <typename T>
    std::string to_stringstream(T&& value) const
    {
        std::stringstream ss;
        if constexpr (std::same_as<bool, std::decay_t<T>>) {
            ss << std::boolalpha;
        }
        ss << std::forward<T>(value);
        return std::move(ss).str();
    }
};

class MAA_UTILS_API LogStream
{
public:
    template <typename... args_t>
    LogStream(std::mutex& m, std::ofstream& s, level lv, bool std_out, args_t&&... args)
        : mutex_(m), stream_(s), lv_(lv), stdout_(std_out)
    {
        stream_props(std::forward<args_t>(args)...);
    }
    LogStream(const LogStream&) = delete;
    LogStream(LogStream&&) noexcept = default;
    ~LogStream()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stdout_) {
            std::cout << stdout_string() << std::endl;
        }
        stream_ << std::move(buffer_).str() << std::endl;
    }

    template <typename T>
    LogStream& operator<<(T&& value)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, separator>) {
            sep_ = std::forward<T>(value);
        }
        else {
            stream(std::forward<T>(value));
        }
        return *this;
    }

private:
    template <typename T>
    void stream(T&& value)
    {
        buffer_ << string_converter_(std::forward<T>(value)) << sep_.str;
    }

    template <typename... args_t>
    void stream_props(args_t&&... args)
    {
#ifdef _WIN32
        int pid = _getpid();
#else
        int pid = ::getpid();
#endif
        auto tid = static_cast<unsigned short>(std::hash<std::thread::id> {}(std::this_thread::get_id()));

        std::string props = MAA_FMT::format("[{}][{}][Px{}][Tx{}]", format_now(), level_str(), pid, tid);
        for (auto&& arg : { args... }) {
            props += MAA_FMT::format("[{}]", arg);
        }
        stream(props);
    }

    std::string stdout_string();
    std::string_view level_str();

private:
    std::mutex& mutex_;
    std::ofstream& stream_;
    const level lv_ = level::error;
    const bool stdout_ = false;
    const StringConverter string_converter_;

    separator sep_ = separator::space;
    std::stringstream buffer_;
};

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

#ifdef __GNUC__
inline std::ostream& operator<<(std::ostream& os, const std::chrono::milliseconds& ms)
{
    return os << ms.count() << "ms";
}
#endif

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    if (v) {
        os << *v;
    }
    else {
        os << "<nullopt>";
    }
    return os;
}

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
