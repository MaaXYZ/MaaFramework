#include "MinicapStream.h"
#include "MinicapDef.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

MinicapStream::~MinicapStream()
{
    quit_ = true;
    if (pull_thread_.joinable()) {
        pull_thread_.join();
    }
}

bool MinicapStream::parse(const json::value& config)
{
    static const json::array kDefaultForwardArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "forward", "tcp:{FOWARD_PORT}", "localabstract:{LOCAL_SOCKET}",
    };
    static constexpr int kDefaultPort = 1313;

    port_ = config.get("prebuilt", "minicap", "stream", "port", kDefaultPort);

    return MinicapBase::parse(config) && parse_argv("ForwardSocket", config, kDefaultForwardArgv, forward_argv_);
}

bool MinicapStream::init(int swidth, int sheight)
{
    LogFunc;

    if (!MinicapBase::init(swidth, sheight)) {
        return false;
    }

    merge_replacement({ { "{FOWARD_PORT}", std::to_string(port_) }, { "{LOCAL_SOCKET}", "minicap" } });

    auto argv_opt = forward_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return false;
    }

    uint32_t width = screencap_helper_.get_w();
    uint32_t height = screencap_helper_.get_h();

    process_handle_ = binary_->invoke_bin(MAA_FMT::format("-P {}x{}@{}x{}/{}", width, height, width, height, 0));

    if (!process_handle_) {
        LogError << "invoke_bin failed";
        return false;
    }

    bool ok = false;

    std::string buffer;
    constexpr int kMaxTry = 50;
    for (int i = 0; i < kMaxTry; ++i) {
        using namespace std::chrono_literals;
        auto res = process_handle_->read(5s);

        if (!res.empty()) {
            LogDebug << "minicap stdout:" << res;
            buffer.append(res);
        }
        if (buffer.find("Allocating") != std::string::npos) {
            ok = true;
            break;
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
    if (!ok) {
        LogError << "minicap stdout:" << buffer;
        return false;
    }

    auto serial_host = argv_replace_["{ADB_SERIAL}"];
    auto shp = serial_host.find(':');
    std::string local = "127.0.0.1";
    if (shp != std::string::npos) {
        local = serial_host.substr(0, shp);
    }

    LogInfo << "minicap try connect to:" << local;

    ClientSockIOFactory io_factory(local, static_cast<unsigned short>(port_));
    stream_handle_ = io_factory.connect();
    if (!stream_handle_) {
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

    LogInfo << VAR(header.version) << VAR(header.size) << VAR(header.pid) << VAR(header.real_width)
            << VAR(header.real_height) << VAR(header.virt_width) << VAR(header.virt_height) << VAR(header.orientation)
            << VAR(header.flags);

    if (header.version != 1 || header.size < sizeof(header)) {
        return false;
    }

    if (header.real_width != width || header.real_height != height || header.virt_width != width ||
        header.virt_height != height) {
        return false;
    }

    if (!read(header.size - sizeof(header))) {
        LogError << "read header failed";
        return false;
    }

    quit_ = false;
    pull_thread_ = std::thread(std::bind(&MinicapStream::working_thread, this));

    return true;
}

std::optional<cv::Mat> MinicapStream::screencap()
{
    std::unique_lock<std::mutex> locker(mutex_);

    using namespace std::chrono_literals;
    cond_.wait_for(locker, 2s); // 等下一帧

    return image_.empty() ? std::nullopt : std::make_optional(image_.clone());
}

std::optional<std::string> MinicapStream::read(size_t count)
{
    if (!stream_handle_) {
        LogError << "stream_handle_ is nullptr";
        return std::nullopt;
    }

    using namespace std::chrono_literals;
    return stream_handle_->read_some(count, 5s);
}

void MinicapStream::working_thread()
{
    LogFunc;

    while (!quit_) {
        auto size_opt = read(4);
        if (!size_opt) {
            LogError << "read size failed";
            std::unique_lock<std::mutex> locker(mutex_);
            image_ = cv::Mat();
            continue;
        }
        auto size = *reinterpret_cast<const uint32_t*>(size_opt->data());

        auto data_opt = read(size);
        if (!data_opt) {
            LogError << "read data failed";
            std::unique_lock<std::mutex> locker(mutex_);
            image_ = cv::Mat();
            continue;
        }

        auto img_opt = screencap_helper_.decode_jpg(*data_opt);

        if (!img_opt || img_opt->empty()) {
            LogError << "decode jpg failed";
            std::unique_lock<std::mutex> locker(mutex_);
            image_ = cv::Mat();
            continue;
        }

        std::unique_lock<std::mutex> locker(mutex_);
        image_ = std::move(*img_opt);
        cond_.notify_all();
    }
}

MAA_CTRL_UNIT_NS_END
