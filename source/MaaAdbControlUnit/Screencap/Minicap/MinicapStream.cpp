#include "MinicapStream.h"

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
    LogDebug;

    std::unique_lock locker(mutex_);
    if (quit_) {
        return std::nullopt;
    }

    using namespace std::chrono_literals;
    cond_.wait_for(locker, 2s); // 等下一帧

    return image_.empty() ? std::nullopt : std::make_optional(image_.clone());
}

std::optional<std::string> MinicapStream::read(size_t count)
{
    if (!sock_ios_) {
        LogError << "sock_ios_ is nullptr";
        return std::nullopt;
    }

    using namespace std::chrono_literals;
    return sock_ios_->read_some(count, 1s);
}

void MinicapStream::create_thread()
{
    quit_ = false;
    pull_thread_ = std::thread(std::bind(&MinicapStream::pulling, this));
}

void MinicapStream::release_thread()
{
    quit_ = true;
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

    auto data = read(sizeof(header));
    if (!data) {
        LogError << "read header failed";
        return false;
    }
    header = *reinterpret_cast<const MinicapHeader*>(data->data());

    LogInfo << VAR(header.version) << VAR(header.size) << VAR(header.pid) << VAR(header.real_width) << VAR(header.real_height)
            << VAR(header.virt_width) << VAR(header.virt_height) << VAR(header.orientation) << VAR(header.flags);

    if (header.version != 1 || header.size < sizeof(header)) {
        return false;
    }

    if (!read(header.size - sizeof(header))) {
        LogError << "read header failed";
        return false;
    }

    return true;
}

void MinicapStream::pulling()
{
    LogFunc;

    while (!quit_) {
        auto size_opt = read(4);
        if (!size_opt) {
            LogError << "read size failed";
            std::unique_lock locker(mutex_);
            image_ = cv::Mat();
            continue;
        }
        auto size = *reinterpret_cast<const uint32_t*>(size_opt->data());

        auto data_opt = read(size);
        if (!data_opt) {
            LogError << "read data failed";
            std::unique_lock locker(mutex_);
            image_ = cv::Mat();
            continue;
        }

        auto img_opt = screencap_helper_.decode_jpg(*data_opt);

        if (!img_opt || img_opt->empty()) {
            LogError << "decode jpg failed";
            std::unique_lock locker(mutex_);
            image_ = cv::Mat();
            continue;
        }

        std::unique_lock locker(mutex_);
        image_ = std::move(*img_opt);
        cond_.notify_all();
    }
}

MAA_CTRL_UNIT_NS_END
