#pragma once

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <type_traits>

#include <meojson/json.hpp>

#include "MaaFramework/MaaDef.h"
#include "MaaFramework/MaaPort.h"

#include "Conf/Conf.h"
#include "Utils/Time.hpp"

namespace cv
{
class Mat;
}

MAA_LOG_NS_BEGIN

#ifdef __GNUC__
inline std::ostream& operator<<(std::ostream& os, const std::chrono::milliseconds& ms)
{
    return os << ms.count() << "ms";
}
#endif

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
    explicit constexpr separator(std::string_view s) noexcept
        : str(s)
    {
    }

    static const separator none;
    static const separator space;
    static const separator tab;
    static const separator newline;
    static const separator comma;

    std::string_view str;
};

template <typename T>
concept has_output_operator = requires { std::declval<std::ostream&>() << std::declval<T>(); };

class MAA_UTILS_API StringConverter
{
public:
    StringConverter(std::filesystem::path dumps_dir)
        : dumps_dir_(std::move(dumps_dir))
    {
    }

public:
    std::string operator()(const std::filesystem::path& path) const;
    std::string operator()(const std::wstring& wstr) const;
    std::string operator()(const cv::Mat& image) const;

    template <typename T>
    std::string operator()(const std::optional<T>& value) const
    {
        if (!value) {
            return "nullopt";
        }
        return this->operator()(*value);
    }

    template <typename T>
    requires has_output_operator<T>
    std::string operator()(const T& value) const
    {
        std::stringstream ss;
        if constexpr (std::same_as<bool, std::decay_t<T>>) {
            ss << std::boolalpha;
        }
        ss << value;
        return std::move(ss).str();
    }

    template <typename T>
    requires(std::is_constructible_v<json::value, T> && !has_output_operator<T>)
    std::string operator()(const T& value) const
    {
        return json::value(value).to_string();
    }

    template <typename T>
    requires(std::is_function_v<std::remove_pointer_t<std::decay_t<T>>>)
    void operator()(T&&) const
    {
        static_assert(!sizeof(T), "Function type is not supported.");
    }

private:
    const std::filesystem::path dumps_dir_;
};

template <typename T>
concept string_convertible = requires { std::declval<StringConverter>()(std::declval<T>()); };

class MAA_UTILS_API LogStream
{
public:
    template <typename... args_t>
    LogStream(std::mutex& m, std::ofstream& s, level lv, bool std_out, std::filesystem::path dumps_dir, args_t&&... args)
        : mutex_(m)
        , stream_(s)
        , lv_(lv)
        , stdout_(std_out)
        , string_converter_(std::move(dumps_dir))
    {
        stream_props(std::forward<args_t>(args)...);
    }

    LogStream(const LogStream&) = delete;
    LogStream(LogStream&&) noexcept = default;

    ~LogStream()
    {
        std::unique_lock lock(mutex_);

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
            stream(std::forward<T>(value), sep_);
        }
        return *this;
    }

    template <typename T>
    LogStream& operator,(T&& value)
    {
        stream(std::forward<T>(value), separator::none);
        return *this;
    }

private:
    template <typename T>
    void stream(T&& value, const separator& sep)
    {
        if constexpr (string_convertible<T>) {
            buffer_ << string_converter_(std::forward<T>(value)) << sep.str;
        }
        else {
            buffer_ << json::serialize(std::forward<T>(value), string_converter_).dumps() << sep.str;
        }
    }

    template <typename... args_t>
    void stream_props(args_t&&... args)
    {
#ifdef _WIN32
        int pid = _getpid();
#else
        int pid = ::getpid();
#endif
        auto tid = static_cast<uint16_t>(std::hash<std::thread::id> {}(std::this_thread::get_id()));

        std::string props = std::format("[{}][{}][Px{}][Tx{}]", format_now(), level_str(), pid, tid);
        for (auto&& arg : { args... }) {
            props += std::format("[{}]", arg);
        }
        stream(props, sep_);
    }

    std::string stdout_string();
    std::string_view level_str();

private:
    std::mutex& mutex_;
    std::ofstream& stream_;
    const level lv_ = level::fatal;
    const bool stdout_ = false;
    const StringConverter string_converter_;

    separator sep_ = separator::space;
    std::stringstream buffer_;
};

MAA_LOG_NS_END
