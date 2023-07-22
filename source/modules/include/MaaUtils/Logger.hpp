#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <thread>
#include <tuple>
#include <type_traits>

#include "Common/MaaConf.h"
#include "Common/MaaTypes.h"
#include "MaaPort.h"
#include "Utils/Format.hpp"
#include "Utils/Locale.hpp"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"
#include "Utils/Time.hpp"

#include <meojson/json.hpp>

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

MAA_NS_BEGIN

#ifdef __GNUC__
std::ostream& operator<<(std::ostream& os, const std::chrono::milliseconds& ms);
#endif
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& v);

template <typename T>
concept has_stream_insertion_operator = requires { std::declval<std::ostream&>() << std::declval<T>(); };
template <typename T>
concept has_value_type = requires { typename std::decay_t<T>::value_type; };
template <typename T>
concept has_key_mapped_type = requires {
    typename std::decay_t<T>::key_type;
    typename std::decay_t<T>::mapped_type;
};

class MAA_UTILS_API Logger
{
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
    class MAA_UTILS_API LogStream
    {
    public:
        template <typename... args_t>
        LogStream(std::mutex& m, std::ofstream& s, level lv, args_t&&... args) : mutex_(m), stream_(s)
        {
            stream(std::boolalpha);
            stream_props(lv, std::forward<args_t>(args)...);
        }
        LogStream(const LogStream&) = delete;
        LogStream(LogStream&&) noexcept = default;
        ~LogStream()
        {
            std::unique_lock<std::mutex> lock(mutex_);

#ifdef MAA_DEBUG
            std::cout << utf8_to_stdout(cout_buffer_.str()) << "\033[0m" << std::endl;
#endif
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
        decltype(auto) to_stream(T&& value)
        {
            if constexpr (std::same_as<std::filesystem::path, std::decay_t<T>>) {
                return path_to_utf8_string(std::forward<T>(value));
            }
            else if constexpr (has_stream_insertion_operator<T>) {
                return std::forward<T>(value);
            }
            else if constexpr (std::is_constructible_v<json::array, T>) {
                return json::array(std::forward<T>(value));
            }
            else if constexpr (std::is_constructible_v<json::object, T>) {
                return json::object(std::forward<T>(value));
            }
            else if constexpr (ranges::input_range<T> && has_value_type<T> && !has_key_mapped_type<T>) {
                using value_type = typename std::decay_t<T>::value_type;
                using value_stream_type = decltype(to_stream(std::declval<value_type>()));

                json::value jtmp;
                for (auto&& val : value) {
                    if constexpr (std::is_constructible_v<json::value, value_stream_type>) {
                        jtmp.emplace(to_stream(std::forward<decltype(val)>(val)));
                    }
                    else {
                        std::stringstream val_ss;
                        val_ss << to_stream(std::forward<decltype(val)>(val));
                        jtmp.emplace(std::move(val_ss).str());
                    }
                }
                return jtmp;
            }
            else if constexpr (ranges::input_range<T> && has_value_type<T> && has_key_mapped_type<T>) {
                using key_type = typename std::decay_t<T>::key_type;
                using key_stream_type = decltype(to_stream(std::declval<key_type>()));
                using mapped_type = typename std::decay_t<T>::mapped_type;
                using mapped_stream_type = decltype(to_stream(std::declval<mapped_type>()));

                json::value jtmp;
                for (auto&& [key, val] : value) {
                    json::object::key_type strkey;
                    if constexpr (std::is_constructible_v<json::object::key_type, key_stream_type>) {
                        strkey = to_stream(key);
                    }
                    else {
                        std::stringstream key_ss;
                        key_ss << to_stream(key);
                        strkey = std::move(key_ss).str();
                    }

                    if constexpr (std::is_constructible_v<json::value, mapped_stream_type>) {
                        jtmp.emplace(std::move(strkey), to_stream(std::forward<decltype(val)>(val)));
                    }
                    else {
                        std::stringstream val_ss;
                        val_ss << to_stream(std::forward<decltype(val)>(val));
                        jtmp.emplace(std::move(strkey), std::move(val_ss).str());
                    }
                }
                return jtmp;
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
            cout_buffer_ << content << sep_.str;
#endif
            buffer_ << std::forward<decltype(content)>(content) << sep_.str;
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

            std::string props = fmt::format("[{}][{}][Px{}][Tx{}]", format_now(), level_str(lv), pid, tid);
            for (auto&& arg : { args... }) {
                props += fmt::format("[{}]", arg);
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
            cout_buffer_ << color;
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
        std::mutex& mutex_;
        std::ofstream& stream_;
        separator sep_ = separator::space;

        std::stringstream buffer_;
        std::stringstream cout_buffer_;
    };

    class MAA_UTILS_API FakeStream
    {
    public:
        template <typename T>
        FakeStream& operator<<(T&& value)
        {
            std::ignore = std::forward<T>(value);
            return *this;
        }
    };

public:
    static Logger& get_instance();
    ~Logger() { flush(); }

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    template <typename... args_t>
    auto debug([[maybe_unused]] args_t&&... args)
    {
#ifdef MAA_DEBUG
        return stream(level::debug, std::forward<args_t>(args)...);
#else
        return FakeStream();
#endif
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

    void start_logging(std::filesystem::path dir);

private:
    template <typename... args_t>
    LogStream stream(level lv, args_t&&... args)
    {
        return LogStream(trace_mutex_, ofs_, lv, std::forward<args_t>(args)...);
    }

    void flush();

private:
    Logger() = default;

    void reinit();
    void rotate();
    void open();
    void log_start();

    LogStream internal_trace();

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

    Logger::LogStream& operator()() { return stream_; }

private:
    Logger::LogStream stream_;
};

template <typename... args_t>
class ScopeLeaveHelper
{
public:
    ScopeLeaveHelper(args_t&&... args) : args_(std::forward<args_t>(args)...) {}
    ~ScopeLeaveHelper()
    {
        std::apply([](auto&&... args) { return Logger::get_instance().trace(std::forward<decltype(args)>(args)...); },
                   std::move(args_))
            << "| leave," << duration_since(start_);
    }

private:
    std::tuple<args_t...> args_;
    std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
};
}

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

#define VAR_RAW(x) "[" << #x << "=" << (x) << "] "
#define VAR(x) MAA_NS::Logger::separator::none << VAR_RAW(x) << MAA_NS::Logger::separator::space
#define VAR_VOIDP_RAW(x) "[" << #x << "=" << ((void*)x) << "] "
#define VAR_VOIDP(x) MAA_NS::Logger::separator::none << VAR_VOIDP_RAW(x) << MAA_NS::Logger::separator::space

MAA_NS_END
