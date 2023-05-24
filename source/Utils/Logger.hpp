#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <thread>
#include <type_traits>

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

template <typename T>
concept has_stream_insertion_operator = requires { std::declval<std::ostream&>() << std::declval<T>(); };

class Logger : public SingletonHolder<Logger>
{
    friend class SingletonHolder<Logger>;

public:
    struct level
    {
        constexpr explicit level(std::string_view s) noexcept : str(s) {}

        static const level debug;
        static const level trace;
        static const level info;
        static const level warn;
        static const level error;

        std::string_view str;
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
        LogStream(std::mutex& m, stream_t& s, level lv, std::string_view flag)
            : lock_(m), stream_(s), lv_(lv), flag_(flag)
        {
            stream_props();
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
            stream_ << std::forward<T>(content) << sep_.str;
        }

        void stream_endl()
        {
#ifdef MAA_DEBUG
            std::cout << std::endl;
#endif
            stream_ << std::endl;
        }

        void stream_props()
        {
#ifdef _WIN32
            int pid = _getpid();
#else
            int pid = ::getpid();
#endif
            auto tid = static_cast<unsigned short>(std::hash<std::thread::id> {}(std::this_thread::get_id()));

            std::string props = std::format("[{}][{}][Px{}][Tx{}][{}]", get_format_time(), lv_.str, pid, tid, flag_);
            stream(props);
        }

    private:
        std::unique_lock<std::mutex> lock_;
        stream_t& stream_;
        level lv_;
        std::string_view flag_;
        separator sep_ = separator::space;
    };

public:
    virtual ~Logger() override { flush(); }

    auto debug(std::string_view flag = {}) { return stream(level::debug, flag); }
    auto trace(std::string_view flag = {}) { return stream(level::trace, flag); }
    auto info(std::string_view flag = {}) { return stream(level::info, flag); }
    auto warn(std::string_view flag = {}) { return stream(level::warn, flag); }
    auto error(std::string_view flag = {}) { return stream(level::error, flag); }

private:
    LogStream<std::ofstream> stream(level lv, std::string_view flag) { return LogStream(trace_mutex_, ofs_, lv, flag); }

    void flush()
    {
        internal_trace() << "-----------------------------";
        internal_trace() << "Flush log";
        internal_trace() << "-----------------------------";

        rotate();
        init();
    }

private:
    Logger()
    {
        rotate();
        init();
        log_start();
    }

    void rotate()
    {
        if (!std::filesystem::exists(log_path_)) {
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

        const std::filesystem::path bak_path = log_path_.parent_path() / "maa_framework.bak.log";
        try {
            std::filesystem::rename(log_path_, bak_path);
        }
        catch (...) {
        }
    }

    void init()
    {
        std::filesystem::create_directories(log_path_.parent_path());
        if (!ofs_.is_open()) {
            ofs_.open(log_path_, std::ios::out | std::ios::app);
        }
    }

    void log_start()
    {
        internal_trace() << "-----------------------------";
        internal_trace() << "MAA Process Start";
        internal_trace() << "Version", MAA_VERSION;
        internal_trace() << "Built at" << __DATE__ << __TIME__;
        internal_trace() << "Log Dir" << log_path_;
        internal_trace() << "-----------------------------";
    }

    LogStream<std::ofstream> internal_trace() { return trace("Logger"); }

private:
    const std::filesystem::path log_path_ =
        MAA_NS::GlabalOption::get_instance().log_dir() / "debug" / "maa_framework.log";
    std::ofstream ofs_;
    std::mutex trace_mutex_;
};

inline constexpr Logger::level Logger::level::debug("DBG");
inline constexpr Logger::level Logger::level::trace("TRC");
inline constexpr Logger::level Logger::level::info("INF");
inline constexpr Logger::level Logger::level::warn("WRN");
inline constexpr Logger::level Logger::level::error("ERR");

inline constexpr Logger::separator Logger::separator::none("");
inline constexpr Logger::separator Logger::separator::space(" ");
inline constexpr Logger::separator Logger::separator::tab("\t");
inline constexpr Logger::separator Logger::separator::newline("\n");
inline constexpr Logger::separator Logger::separator::comma(",");

class ScopeEnterHelper
{
public:
    ScopeEnterHelper(std::string_view flag) : stream_(Logger::get_instance().info(flag)) {}
    ~ScopeEnterHelper() { stream_ << "| Enter"; }

    Logger::LogStream<std::ofstream>& operator()() { return stream_; }

private:
    Logger::LogStream<std::ofstream> stream_;
};

class ScopeLeaveHelper
{
public:
    ScopeLeaveHelper(std::string_view flag) : flag_(flag) {}
    ~ScopeLeaveHelper()
    {
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_);
        Logger::get_instance().info(flag_) << "| Leave," << duration;
    }

private:
    std::string_view flag_;
    std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
};

inline constexpr std::string_view maa_pertty_func(std::string_view func)
{
    size_t end = func.find('(');
    size_t beg = func.rfind(' ', end);
    return func.substr(beg + 1, end - beg - 1);
}

#define MAA_FUNCTION MAA_NS::maa_pertty_func(std::source_location::current().function_name())
#define LOG_FLAG MAA_FUNCTION

#define LogDebug MAA_NS::Logger::get_instance().debug(LOG_FLAG)
#define LogTrace MAA_NS::Logger::get_instance().trace(LOG_FLAG)
#define LogInfo MAA_NS::Logger::get_instance().info(LOG_FLAG)
#define LogWarn MAA_NS::Logger::get_instance().warn(LOG_FLAG)
#define LogError MAA_NS::Logger::get_instance().error(LOG_FLAG)

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)
#define ScopeHeplerName _CatVarNameWithLine(log_scope_)

#define LogFunc                                         \
    MAA_NS::ScopeLeaveHelper ScopeHeplerName(LOG_FLAG); \
    MAA_NS::ScopeEnterHelper(LOG_FLAG)()

#define VAR(x) MAA_NS::Logger::separator::none << "[" << #x << "=" << (x) << "] " << MAA_NS::Logger::separator::space
#define VAR_VOIDP(x) \
    MAA_NS::Logger::separator::none << "[" << #x << "=" << (void*)(x) << "] " << MAA_NS::Logger::separator::space

MAA_NS_END
