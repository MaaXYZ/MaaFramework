#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <thread>
#include <tuple>
#include <type_traits>

#include "Base/SingletonHolder.hpp"
#include "Common/MaaConf.h"
#include "Common/MaaTypes.h"
#include "Locale.hpp"
#include "Platform.hpp"
#include "Ranges.hpp"
#include "Time.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

MAA_NS_BEGIN

template <typename T>
concept has_stream_insertion_operator = requires { std::declval<std::ostream&>() << std::declval<T>(); };

class Logger : public SingletonHolder<Logger>
{
    friend class SingletonHolder<Logger>;

public:
    enum class level
    {
        debug,
        trace,
        info,
        warn,
        error,
    };

    struct separator
    {
        constexpr separator(std::string_view s) noexcept : str(s) {}

        static const separator none;
        static const separator space;
        static const separator tab;
        static const separator newline;
        static const separator comma;

        std::string_view str;
    };

public:
    template <typename stream_t>
    class LogStream
    {
    public:
        template <typename... args_t>
        LogStream(std::mutex& m, stream_t& s, level lv, args_t&&... args) : lock_(m), stream_(s)
        {
            stream_props(lv, std::forward<args_t>(args)...);
        }
        LogStream(const LogStream&) = delete;
        LogStream(LogStream&&) = default;
        ~LogStream() { stream_endl(); }

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
        decltype(auto) to_stream(T&& value)
        {
            if constexpr (has_stream_insertion_operator<T>) {
                return std::forward<T>(value);
            }
            else if constexpr (std::is_constructible_v<json::array, T>) {
                return json::array(std::forward<T>(value));
            }
            else if constexpr (std::is_constructible_v<json::object, T>) {
                return json::object(std::forward<T>(value));
            }
            else {
                static_assert(!sizeof(T), "Unsupported type");
            }
        }

        template <typename T>
        void stream(T&& value)
        {
            auto&& content = to_stream(std::forward<T>(value));

#ifdef MAA_DEBUG
            std::cout << content << sep_.str;
#endif
            stream_ << std::forward<decltype(content)>(content) << sep_.str;
        }

        void stream_endl()
        {
#ifdef MAA_DEBUG
            std::cout << "\033[0m" << std::endl;
#endif
            stream_ << std::endl;
        }

        template <typename... args_t>
        void stream_props(level lv, args_t&&... args)
        {
#ifdef MAA_DEBUG
            print_color(lv);
#endif

#ifdef _WIN32
            int pid = _getpid();
#else
            int pid = ::getpid();
#endif
            auto tid = static_cast<unsigned short>(std::hash<std::thread::id> {}(std::this_thread::get_id()));

            std::string props = std::format("[{}][{}][Px{}][Tx{}]", get_format_time(), level_str(lv), pid, tid);
            for (auto&& arg : { args... }) {
                props += std::format("[{}]", arg);
            }
            stream(props);
        }

#ifdef MAA_DEBUG
        void print_color(level lv)
        {
            std::string_view color;

            switch (lv) {
            case level::debug:
            case level::trace:
                break;
            case level::info:
                color = "\033[32m";
                break;
            case level::warn:
                color = "\033[33m";
                break;
            case level::error:
                color = "\033[31m";
                break;
            }
            std::cout << color;
        }
#endif

        constexpr std::string_view level_str(level lv)
        {
            switch (lv) {
            case level::debug:
                return "DBG";
            case level::trace:
                return "TRC";
            case level::info:
                return "INF";
            case level::warn:
                return "WRN";
            case level::error:
                return "ERR";
            }
            return "NoLV";
        }

    private:
        std::unique_lock<std::mutex> lock_;
        stream_t& stream_;
        separator sep_ = separator::space;
    };

public:
    virtual ~Logger() override { flush(); }

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
    template <typename... args_t>
    auto info(args_t&&... args)
    {
        return stream(level::info, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto warn(args_t&&... args)
    {
        return stream(level::warn, std::forward<args_t>(args)...);
    }
    template <typename... args_t>
    auto error(args_t&&... args)
    {
        return stream(level::error, std::forward<args_t>(args)...);
    }

    void start_logging(std::filesystem::path dir)
    {
        log_path_ = std::move(dir) / "maa.log";
        reinit();
    }

private:
    template <typename... args_t>
    LogStream<std::ofstream> stream(level lv, args_t&&... args)
    {
        return LogStream(trace_mutex_, ofs_, lv, std::forward<args_t>(args)...);
    }

    void flush()
    {
        internal_trace() << "-----------------------------";
        internal_trace() << "Flush log";
        internal_trace() << "-----------------------------";

        rotate();
        open();
    }

private:
    Logger() = default;

    void reinit()
    {
        rotate();
        open();
        log_start();
    }

    void rotate()
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

    void open()
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

    void log_start()
    {
        internal_trace() << "-----------------------------";
        internal_trace() << "MAA Process Start";
        internal_trace() << "Version" << MAA_VERSION;
        internal_trace() << "Built at" << __DATE__ << __TIME__;
        internal_trace() << "Log Path" << log_path_;
        internal_trace() << "-----------------------------";
    }

    LogStream<std::ofstream> internal_trace() { return trace("Logger"); }

private:
    std::filesystem::path log_path_;
    std::ofstream ofs_;
    std::mutex trace_mutex_;
};

inline constexpr Logger::separator Logger::separator::none("");
inline constexpr Logger::separator Logger::separator::space(" ");
inline constexpr Logger::separator Logger::separator::tab("\t");
inline constexpr Logger::separator Logger::separator::newline("\n");
inline constexpr Logger::separator Logger::separator::comma(",");

namespace LogUtils
{
class ScopeEnterHelper
{
public:
    template <typename... args_t>
    ScopeEnterHelper(args_t&&... args) : stream_(Logger::get_instance().trace(std::forward<args_t>(args)...))
    {}
    ~ScopeEnterHelper() { stream_ << "| enter"; }

    Logger::LogStream<std::ofstream>& operator()() { return stream_; }

private:
    Logger::LogStream<std::ofstream> stream_;
};

template <typename... args_t>
class ScopeLeaveHelper
{
public:
    ScopeLeaveHelper(args_t&&... args) : args_(std::forward<args_t>(args)...) {}
    ~ScopeLeaveHelper()
    {
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_).count();
        std::apply([](auto&&... args) { return Logger::get_instance().trace(std::forward<decltype(args)>(args)...); },
                   std::move(args_))
            << "| leave," << duration << "ms";
    }

private:
    std::tuple<args_t...> args_;
    std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
};
}

inline constexpr std::string_view pertty_file(std::string_view file)
{
    size_t pos = file.find_last_of(std::filesystem::path::preferred_separator);
    return file.substr(pos + 1, file.size());
}

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define MAA_FILE MAA_NS::pertty_file(__FILE__)
#define MAA_LINE std::string_view("L" LINE_STRING)
#ifdef _MSC_VER
#define MAA_FUNCTION std::string_view(__FUNCTION__)
#else
#define MAA_FUNCTION std::string_view(__PRETTY_FUNCTION__)
#endif
#define LOG_ARGS MAA_FILE, MAA_LINE, MAA_FUNCTION

#define LogDebug MAA_NS::Logger::get_instance().debug(LOG_ARGS)
#define LogTrace MAA_NS::Logger::get_instance().trace(LOG_ARGS)
#define LogInfo MAA_NS::Logger::get_instance().info(LOG_ARGS)
#define LogWarn MAA_NS::Logger::get_instance().warn(LOG_ARGS)
#define LogError MAA_NS::Logger::get_instance().error(LOG_ARGS)

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)
#define LogScopeHeplerName _CatVarNameWithLine(log_scope_)

#define LogFunc                                                      \
    MAA_NS::LogUtils::ScopeLeaveHelper LogScopeHeplerName(LOG_ARGS); \
    MAA_NS::LogUtils::ScopeEnterHelper(LOG_ARGS)()

#define VAR(x) MAA_NS::Logger::separator::none << "[" << #x << "=" << x << "] " << MAA_NS::Logger::separator::space
#define VAR_VOIDP(x) \
    MAA_NS::Logger::separator::none << "[" << #x << "=" << (void*)x << "] " << MAA_NS::Logger::separator::space

MAA_NS_END
