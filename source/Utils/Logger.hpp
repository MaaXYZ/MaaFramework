#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include "Common/MaaConf.h"
#include "Common/MaaTypes.h"
#include "Locale.hpp"
#include "Option/GlobalOption.h"
#include "Platform.hpp"
#include "Ranges.hpp"
#include "SingletonHolder.hpp"
#include "Time.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

MAA_NS_BEGIN

template <typename Stream, typename T>
concept has_stream_insertion_operator = requires { std::declval<Stream&>() << std::declval<T>(); };
template <typename T>
concept enum_could_to_string = requires { enum_to_string(std::declval<T>()); };

// is_reference_wrapper
template <typename T>
struct is_reference_wrapper : std::false_type
{};
template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{};
template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>&> : std::true_type
{};
template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>&&> : std::true_type
{};
template <typename T>
inline constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

// from_reference_wrapper
template <typename T>
struct from_reference_wrapper
{
    typedef T type;
};
template <typename T>
struct from_reference_wrapper<std::reference_wrapper<T>>
{
    typedef typename from_reference_wrapper<T&>::type type;
};
template <typename T>
struct from_reference_wrapper<std::reference_wrapper<T>&>
{
    typedef typename from_reference_wrapper<T&>::type type;
};
template <typename T>
struct from_reference_wrapper<std::reference_wrapper<T>&&>
{
    typedef typename from_reference_wrapper<T&>::type type;
};
template <typename T>
using from_reference_wrapper_t = typename from_reference_wrapper<T>::type;

// to_reference_wrapper
template <typename T>
struct to_reference_wrapper
{
    typedef T type;
};
template <typename T>
struct to_reference_wrapper<T&>
{
    typedef std::reference_wrapper<T> type;
};
template <typename T>
struct to_reference_wrapper<std::reference_wrapper<T>>
{
    typedef typename to_reference_wrapper<T&>::type type;
};
template <typename T>
struct to_reference_wrapper<std::reference_wrapper<T>&>
{
    typedef typename to_reference_wrapper<T&>::type type;
};
template <typename T>
struct to_reference_wrapper<std::reference_wrapper<T>&&>
{
    typedef typename to_reference_wrapper<T&>::type type;
};
template <typename T>
using to_reference_wrapper_t = typename to_reference_wrapper<T>::type;

template <typename T>
struct remove_cvref
{
    typedef std::remove_cvref_t<from_reference_wrapper_t<std::remove_cvref_t<T>>> type;
};
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T>
constexpr from_reference_wrapper_t<T> convert_reference_wrapper(T&& x)
{
    return x;
}

class toansi_ostream
{
    std::reference_wrapper<std::ostream> ofs_;

public:
    toansi_ostream(toansi_ostream&&) = default;
    toansi_ostream(const toansi_ostream&) = default;
    toansi_ostream& operator=(toansi_ostream&&) = default;
    toansi_ostream& operator=(const toansi_ostream&) = default;

    toansi_ostream(std::ostream& stream) : ofs_(stream) {}

    toansi_ostream(std::reference_wrapper<std::ostream> stream) : ofs_(stream) {}

    template <typename T>
    requires has_stream_insertion_operator<std::ostream, T>
    toansi_ostream& operator<<(T&& v)
    {
        if constexpr (std::convertible_to<T, std::string_view>) {
            ofs_.get() << utf8_to_ansi(std::forward<T>(v));
        }
        else {
            ofs_.get() << std::forward<T>(v);
        }
        return *this;
    }

    toansi_ostream& operator<<(std::ostream& (*pf)(std::ostream&))
    {
        ofs_.get() << pf;
        return *this;
    }
};

template <typename... Args>
class ostreams
{
    std::tuple<Args...> ofs_s;

public:
    ostreams(ostreams&&) = default;
    ostreams(const ostreams&) = default;
    ostreams& operator=(ostreams&&) = default;
    ostreams& operator=(const ostreams&) = default;

    ostreams(Args&&... args) : ofs_s(std::forward<Args>(args)...) {}

    template <typename T>
    requires has_stream_insertion_operator<std::ostream, T>
    ostreams& operator<<(T&& x)
    {
        streams_put(ofs_s, x, std::index_sequence_for<Args...> {});
        return *this;
    }
    template <typename Tuple, typename T, size_t... Is>
    static void streams_put(Tuple& t, T&& x, std::index_sequence<Is...>)
    {
        ((convert_reference_wrapper(std::get<Is>(t)) << std::forward<T>(x)), ...);
    }

    ostreams& operator<<(std::ostream& (*pf)(std::ostream&))
    {
        streams_put(ofs_s, pf, std::index_sequence_for<Args...> {});
        return *this;
    }
    template <typename Tuple, size_t... Is>
    static void streams_put(Tuple& t, std::ostream& (*pf)(std::ostream&), std::index_sequence<Is...>)
    {
        ((convert_reference_wrapper(std::get<Is>(t)) << pf), ...);
    }
};

template <typename... Args>
ostreams(Args&&...) -> ostreams<to_reference_wrapper_t<Args>...>;

class Logger : public SingletonHolder<Logger>
{
public:
    struct separator
    {
        constexpr separator() = default;
        constexpr separator(const separator&) = default;
        constexpr separator(separator&&) noexcept = default;
        constexpr explicit separator(std::string_view s) noexcept : str(s) {}
        constexpr separator& operator=(const separator&) = default;
        constexpr separator& operator=(separator&&) noexcept = default;
        constexpr separator& operator=(std::string_view s) noexcept
        {
            str = s;
            return *this;
        }

        static const separator none;
        static const separator space;
        static const separator tab;
        static const separator newline;
        static const separator comma;

        std::string_view str;
    };

    struct level
    {
        constexpr level(const level&) = default;
        constexpr level(level&&) noexcept = default;
        constexpr explicit level(std::string_view s) noexcept : str(s) {}
        constexpr level& operator=(const level&) = default;
        constexpr level& operator=(level&&) noexcept = default;
        constexpr level& operator=(std::string_view s) noexcept
        {
            str = s;
            return *this;
        }

        static const level debug;
        static const level trace;
        static const level info;
        static const level warn;
        static const level error;

        std::string_view str;
    };

    template <typename stream_t>
    class LogStream
    {
    public:
        template <typename T>
        LogStream& operator<<(T&& arg)
        {
            if constexpr (std::same_as<separator, remove_cvref_t<T>>) {
                m_sep = std::forward<T>(arg);
            }
            else {
                // 如果是 level，则不输出 separator
                if constexpr (!std::same_as<Logger::level, remove_cvref_t<T>>) {
                    stream_put(ofs_, m_sep.str);
                }
                stream_put(ofs_, std::forward<T>(arg));
            }
            return *this;
        }

        template <typename _stream_t = stream_t>
        LogStream(std::mutex& mtx, _stream_t&& ofs, Logger::level lv) : m_trace_lock(mtx), ofs_(ofs)
        {
            *this << lv;
        }
        template <typename _stream_t = stream_t, typename... Args>
        LogStream(std::mutex& mtx, _stream_t&& ofs, Logger::level lv, Args&&... buff) : m_trace_lock(mtx), ofs_(ofs)
        {
            ((*this << lv) << ... << std::forward<Args>(buff));
        }
        LogStream(LogStream&&) = delete;
        LogStream(const LogStream&) = delete;
        LogStream& operator=(LogStream&&) = delete;
        LogStream& operator=(const LogStream&) = delete;

        ~LogStream() { ofs_ << std::endl; }

    private:
        template <typename Stream, typename T>
        static Stream& stream_put(Stream& s, T&& v)
        {
            if constexpr (std::same_as<std::filesystem::path, remove_cvref_t<T>>) {
                s << path_to_utf8_string(std::forward<T>(v));
            }
            else if constexpr (std::same_as<Logger::level, remove_cvref_t<T>>) {
                constexpr int buff_len = 128;
                char buff[buff_len] = { 0 };
#ifdef _WIN32
#ifdef _MSC_VER
                sprintf_s(buff, buff_len,
#else  // ! _MSC_VER
                sprintf(buff,
#endif // END _MSC_VER
                          "[%s][%s][Px%x][Tx%lx]", get_format_time().c_str(), v.str.data(), _getpid(),
                          ::GetCurrentThreadId());
#else  // ! _WIN32
                sprintf(buff, "[%s][%s][Px%x][Tx%hx]", get_format_time().c_str(), v.str.data(), ::getpid(),
                        static_cast<unsigned short>(std::hash<std::thread::id> {}(std::this_thread::get_id())));
#endif // END _WIN32
                s << buff;
            }
            else if constexpr (std::is_enum_v<T> && enum_could_to_string<T>) {
                s << ::enum_to_string(std::forward<T>(v));
            }
            else if constexpr (has_stream_insertion_operator<Stream, T>) {
                s << std::forward<T>(v);
            }
            else if constexpr (std::constructible_from<std::string, T>) {
                s << std::string(std::forward<T>(v));
            }
            else if constexpr (ranges::input_range<T>) {
                s << "[";
                std::string_view comma_space {};
                for (const auto& elem : std::forward<T>(v)) {
                    s << comma_space;
                    stream_put(s, elem);
                    comma_space = ", ";
                }
                s << "]";
            }
            else {
                MAA_STATIC_ASSERT_FALSE(
                    "unsupported type, one of the following expected\n"
                    "\t1. those can be converted to string;\n"
                    "\t2. those can be inserted to stream with operator<< directly;\n"
                    "\t3. container or nested container containing 1. 2. or 3. and iterable with range-based for",
                    Stream, T);
            }
            return s;
        }

        separator m_sep = separator::space;
        std::unique_lock<std::mutex> m_trace_lock;
        stream_t ofs_;
    };

    // template <typename stream_t>
    // LogStream(std::mutex&, stream_t&) -> LogStream<stream_t&>;

    // template <typename stream_t>
    // LogStream(std::mutex&, stream_t&&) -> LogStream<stream_t>;

    template <typename stream_t, typename... Args>
    LogStream(std::mutex&, stream_t&, Args&&...) -> LogStream<stream_t&>;

    template <typename stream_t, typename... Args>
    LogStream(std::mutex&, stream_t&&, Args&&...) -> LogStream<stream_t>;

public:
    virtual ~Logger() override { flush(); }

    template <typename T>
    auto operator<<(T&& arg)
    {
        if (!ofs_ || !ofs_.is_open()) {
            ofs_ = std::ofstream(log_path_, std::ios::out | std::ios::app);
        }
        if constexpr (std::same_as<level, remove_cvref_t<T>>) {
#ifdef MAA_DEBUG
            return LogStream(trace_mutex_, ostreams { toansi_ostream(std::cout), ofs_ }, arg);
#else
            return LogStream(trace_mutex_, ofs_, arg);
#endif
        }
        else {
#ifdef MAA_DEBUG
            return LogStream(trace_mutex_, ostreams { toansi_ostream(std::cout), ofs_ }, level::trace, arg);
#else
            return LogStream(trace_mutex_, ofs_, level::trace, arg);
#endif
        }
    }

    template <typename... Args>
    inline void debug([[maybe_unused]] Args&&... args)
    {
#ifdef MAA_DEBUG
        log(level::debug, std::forward<Args>(args)...);
#endif
    }
    template <typename... Args>
    inline void trace(Args&&... args)
    {
        log(level::trace, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void info(Args&&... args)
    {
        log(level::info, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void warn(Args&&... args)
    {
        log(level::warn, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void error(Args&&... args)
    {
        log(level::error, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void log(level lv, Args&&... args)
    {
        ((*this << lv) << ... << std::forward<Args>(args));
    }

    void flush()
    {
        std::unique_lock<std::mutex> m_trace_lock(trace_mutex_);
        if (ofs_.is_open()) {
            ofs_.close();
        }
    }

private:
    friend class SingletonHolder<Logger>;

    Logger()
    {
        std::filesystem::create_directories(log_path_.parent_path());
        check_filesize_and_remove();
        log_init_info();
    }

    void check_filesize_and_remove() const
    {
        if (std::filesystem::exists(log_path_)) {
            return;
        }

        constexpr uintmax_t MaxLogSize = 4ULL * 1024 * 1024;
        const uintmax_t log_size = std::filesystem::file_size(log_path_);
        if (log_size < MaxLogSize) {
            return;
        }

        const std::filesystem::path bak_path = log_path_.parent_path() / "maa_framework.bak.log";
        try {
            std::filesystem::rename(log_path_, bak_path);
        }
        catch (...) {
        }
    }

    void log_init_info()
    {
        trace("-----------------------------");
        trace("MAA Process Start");
        trace("Version", MAA_VERSION);
        trace("Built at", __DATE__, __TIME__);
        trace("Log Dir", log_path_);
        trace("-----------------------------");
    }

    const std::filesystem::path log_path_ =
        MAA_NS::GlabalOption::get_instance().log_dir() / "debug" / "maa_framework.log";
    std::mutex trace_mutex_;
    std::ofstream ofs_;
};

inline constexpr Logger::separator Logger::separator::none;
inline constexpr Logger::separator Logger::separator::space(" ");
inline constexpr Logger::separator Logger::separator::tab("\t");
inline constexpr Logger::separator Logger::separator::newline("\n");
inline constexpr Logger::separator Logger::separator::comma(",");

inline constexpr Logger::level Logger::level::debug("DBG");
inline constexpr Logger::level Logger::level::trace("TRC");
inline constexpr Logger::level Logger::level::info("INF");
inline constexpr Logger::level Logger::level::warn("WRN");
inline constexpr Logger::level Logger::level::error("ERR");

class LoggerAux
{
public:
    explicit LoggerAux(std::string_view func_name)
        : m_func_name(func_name), m_start_time(std::chrono::steady_clock::now())
    {
        Logger::get_instance().trace(m_func_name, "| enter");
    }
    ~LoggerAux()
    {
        const auto duration = std::chrono::steady_clock::now() - m_start_time;
        Logger::get_instance().trace(m_func_name, "| leave,",
                                     std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), "ms");
    }
    LoggerAux(const LoggerAux&) = default;
    LoggerAux(LoggerAux&&) = default;
    LoggerAux& operator=(const LoggerAux&) = default;
    LoggerAux& operator=(LoggerAux&&) = default;

private:
    std::string m_func_name;
    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
};

#ifndef _MSC_VER
inline constexpr std::string_view summarize_pretty_function(std::string_view pf) // can be consteval?
{
    // unable to handle something like std::function<void(void)> gen_func()
    const auto paren = pf.find_first_of('(');
    if (paren != std::string_view::npos) pf.remove_suffix(pf.size() - paren);
    const auto space = pf.find_last_of(' ');
    if (space != std::string_view::npos) pf.remove_prefix(space + 1);
    return pf;
}
// TODO: use std::source_location
#define MAA_FUNCTION "[ " MAA_NS::summarize_pretty_function(__PRETTY_FUNCTION__) " ]"
#else
#define MAA_FUNCTION "[ " __FUNCTION__ " ]"
#endif

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)

#define Log MAA_NS::Logger::get_instance()
#define LogDebug Log << MAA_NS::Logger::level::debug
#define LogTrace Log << MAA_NS::Logger::level::trace
#define LogInfo Log << MAA_NS::Logger::level::info
#define LogWarn Log << MAA_NS::Logger::level::warn
#define LogError Log << MAA_NS::Logger::level::error

#define LogTraceScope MAA_NS::LoggerAux _CatVarNameWithLine(_func_aux_)

#define LogTraceFunction LogTraceScope(MAA_FUNCTION)
#define VAR(x) "[", #x, ":", (x), "]"
#define VAR_VOIDP(x) "[", #x, ":", (void*)(x), "]"

MAA_NS_END
