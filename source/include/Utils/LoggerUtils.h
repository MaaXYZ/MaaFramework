#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
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

MAA_LOG_NS_BEGIN

#ifdef __GNUC__
inline std::ostream& operator<<(std::ostream& os, const std::chrono::milliseconds& ms)
{
    return os << ms.count() << "ms";
}
#endif

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    if (v) {
        os << *v;
    }
    else {
        os << "<nullopt>";
    }
    return os;
}

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

template <typename T>
concept has_output_operator = requires { std::declval<std::ostream&>() << std::declval<T>(); };

struct StringConverter
{
    template <typename T>
    static constexpr bool is_convertible = std::same_as<std::filesystem::path, std::decay_t<T>> ||
                                           std::same_as<std::wstring, std::decay_t<T>> || has_output_operator<T>;

    template <typename T>
    std::string operator()(T&& value) const
    {
        if constexpr (std::is_function_v<std::remove_pointer_t<std::decay_t<T>>>) {
            static_assert(!sizeof(T), "Function type is not supported");
        }
        else if constexpr (std::same_as<std::filesystem::path, std::decay_t<T>>) {
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
    const level lv_ = level::fatal;
    const bool stdout_ = false;
    const StringConverter string_converter_ {};

    separator sep_ = separator::space;
    std::stringstream buffer_;
};

MAA_LOG_NS_END
