#include "MinicapStream.h"

#include <algorithm>
#include <cstring>
#include <format>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MinicapDef.h"

MAA_CTRL_UNIT_NS_BEGIN

MinicapStream::~MinicapStream()
{
    release_thread();

    sock_ios_ = nullptr;
    pipe_ios_ = nullptr;

    deinit_binary();
}

bool MinicapStream::parse(const json::value& config)
{
    static const json::array kDefaultForwardArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "forward", "tcp:{FOWARD_PORT}", "localabstract:{LOCAL_SOCKET}",
    };
    static constexpr int kDefaultPort = 1313;

    port_ = config.get("prebuilt", "minicap", "stream", "port", kDefaultPort);

    return MinicapBase::parse(config) && parse_command("ForwardSocket", config, kDefaultForwardArgv, forward_argv_);
}

bool MinicapStream::init()
{
    LogFunc;

    if (!init_binary()) {
        return false;
    }

    if (!connect_and_check()) {
        return false;
    }

    create_thread();

    return true;
}

std::optional<cv::Mat> MinicapStream::screencap()
{
    std::unique_lock locker(mutex_);
    if (quit_) {
        return std::nullopt;
    }

    using namespace std::chrono_literals;
    cond_.wait_for(locker, 2s); // 等下一帧

    return image_.empty() ? std::nullopt : std::make_optional(image_.clone());
}

std::optional<std::string> MinicapStream::read_exact(size_t count)
{
    if (!sock_ios_) {
        LogError << "sock_ios_ is nullptr";
        return std::nullopt;
    }

    using namespace std::chrono_literals;
    auto data = sock_ios_->read_some(count, 1s);
    return data.size() == count ? std::make_optional(std::move(data)) : std::nullopt;
}

std::optional<cv::Mat> MinicapStream::read_frame()
{
    auto size_data = read_exact(sizeof(uint32_t));
    if (!size_data) {
        return std::nullopt;
    }

    uint32_t size = 0;
    std::memcpy(&size, size_data->data(), sizeof(size));

    auto data = read_exact(size);
    if (!data) {
        return std::nullopt;
    }

    return screencap_helper_.decode_jpg(*data, false);
}

void MinicapStream::create_thread()
{
    quit_ = false;
    pull_thread_ = std::thread(std::bind(&MinicapStream::pulling, this));
}

void MinicapStream::release_thread()
{
    quit_ = true;
    cond_.notify_all();
    if (pull_thread_.joinable()) {
        pull_thread_.join();
    }
}

bool MinicapStream::connect_and_check()
{
    merge_replacement({ { "{FOWARD_PORT}", std::to_string(port_) }, { "{LOCAL_SOCKET}", "minicap" } });

    auto argv_opt = forward_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto startup_output_opt = startup_and_read_pipe(*argv_opt);
    if (!startup_output_opt) {
        return false;
    }

    pipe_ios_ = binary_->invoke_bin(std::format("-P {}x{}@{}x{}/{}", display_width_, display_height_, display_width_, display_height_, 0));

    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    constexpr std::string_view kFlag = "Allocating";
    using namespace std::chrono_literals;
    std::string invoke_output = pipe_ios_->read_until(kFlag, 10s);
    if (!invoke_output.ends_with(kFlag)) {
        LogError << "read_until failed" << VAR(invoke_output);
        return false;
    }

    auto serial_host = argv_replace_["{ADB_SERIAL}"];
    auto shp = serial_host.find(':');
    std::string local = "127.0.0.1";
    if (shp != std::string::npos) {
        local = serial_host.substr(0, shp);
    }

    LogInfo << "minicap try to connect" << VAR(local) << VAR(port_);

    ClientSockIOFactory io_factory(local, static_cast<uint16_t>(port_));
    sock_ios_ = io_factory.connect();
    if (!sock_ios_) {
        return false;
    }

    LogInfo << "connected to screencap";

    // TODO: 解决大端底的情况
    MinicapHeader header;

    auto data = read_exact(sizeof(header));
    if (!data) {
        LogError << "read header failed";
        return false;
    }
    std::memcpy(&header, data->data(), sizeof(header));

    LogInfo << VAR(header.version) << VAR(header.size) << VAR(header.pid) << VAR(header.real_width) << VAR(header.real_height)
            << VAR(header.virt_width) << VAR(header.virt_height) << VAR(header.orientation) << VAR(header.flags);

    if (header.version != 1 || header.size < sizeof(header)) {
        return false;
    }

    if (!read_exact(header.size - sizeof(header))) {
        LogError << "read header failed";
        return false;
    }

    return true;
}

void MinicapStream::pulling()
{
    LogFunc;

    using namespace std::chrono_literals;

    constexpr auto kBackoffBase = 10ms;
    constexpr auto kBackoffMax = 1000ms;
    constexpr auto kLogInterval = 1min;

    size_t consecutive_failures = 0;
    auto backoff = kBackoffBase;
    auto next_log_time = std::chrono::steady_clock::time_point::min();

    while (!quit_) {
        auto image = read_frame();
        if (image && !image->empty()) {
            consecutive_failures = 0;
            backoff = kBackoffBase;

            std::unique_lock locker(mutex_);
            image_ = std::move(*image);
            cond_.notify_all();
            continue;
        }

        if (quit_) {
            break;
        }

        ++consecutive_failures;
        auto now = std::chrono::steady_clock::now();
        if (consecutive_failures == 1) {
            LogError << "minicap stream read/decode failed, retrying with backoff";
            next_log_time = now + kLogInterval;
        }
        else if (now >= next_log_time) {
            LogError << "minicap stream still failing" << VAR(consecutive_failures);
            next_log_time = now + kLogInterval;
        }

        std::unique_lock locker(mutex_);
        image_.release();
        cond_.notify_all();
        cond_.wait_for(locker, backoff, [this]() { return quit_.load(); });
        backoff = std::min(backoff * 2, kBackoffMax);
    }
}

MAA_CTRL_UNIT_NS_END
