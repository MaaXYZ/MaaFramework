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
    return MinicapBase::parse(config) && parse_argv("ForwardSocket", config, forward_argv_);
}

bool MinicapStream::init(int swidth, int sheight)
{
    LogFunc;

    if (!MinicapBase::init(swidth, sheight)) {
        return false;
    }

    // TODO: 也许可以允许配置?
    merge_replacement({ { "{FOWARD_PORT}", "1313" }, { "{LOCAL_SOCKET}", "minicap" } });
    auto cmd_ret = command(forward_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return false;
    }

    uint32_t width = screencap_helper_.get_w();
    uint32_t height = screencap_helper_.get_h();

    process_handle_ = binary_->invoke_bin(MAA_FMT::format("-P {}x{}@{}x{}/{}", width, height, width, height, 0));

    if (!process_handle_) {
        LogError << "invoke screencap failed";
        return false;
    }

    std::string outputBuffer;
    int i = 50;
    while (--i) {
        auto res = process_handle_->read(5);
        if (res.length() > 0) {
            LogInfo << "minicap stdout:" << res;
            outputBuffer.append(res);
        }
        if (outputBuffer.find("Allocating") != std::string::npos) {
            break;
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }

    if (i == 0) {
        return false;
    }

    auto serial_host = argv_replace_["{ADB_SERIAL}"];
    auto shp = serial_host.find(':');
    std::string local = "127.0.0.1";
    if (shp != std::string::npos) {
        local = serial_host.substr(0, shp);
    }

    LogInfo << "minicap try connect to:" << local;

    stream_handle_ = io_ptr_->tcp(local, 1313);

    if (!stream_handle_) {
        return false;
    }

    LogInfo << "connected to screencap";

    // TODO: 解决大端底的情况
    MinicapHeader header;

    if (!take_out(&header, sizeof(header))) {
        LogError << "take_out header failed";
        return false;
    }

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

    if (!take_out(nullptr, header.size - sizeof(header))) {
        LogError << "take_out header failed";
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

bool MinicapStream::read_until(std::string& buffer, size_t size)
{
    // LogFunc;

    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();

    while (buffer.size() < size && duration_since(start) < 5s) {
        auto ret = stream_handle_->read(2, size - buffer.size());
        buffer += std::move(ret);
    }

    return buffer.size() == size;
}

bool MinicapStream::take_out(void* out, size_t size)
{
    // LogFunc;

    std::string buffer;
    if (!read_until(buffer, size)) {
        return false;
    }
    if (out) {
        memcpy(out, buffer.data(), size);
    }
    return true;
}

void MinicapStream::working_thread()
{
    LogFunc;

    while (!quit_) {
        uint32_t size = 0;
        if (!take_out(&size, 4)) {
            LogError << "take_out size failed";
            std::unique_lock<std::mutex> locker(mutex_);
            image_ = cv::Mat();
            continue;
        }

        std::string buffer;
        if (!read_until(buffer, size)) {
            LogError << "read_until size failed";
            std::unique_lock<std::mutex> locker(mutex_);
            image_ = cv::Mat();
            continue;
        }

        auto img_opt = screencap_helper_.decode_jpg(buffer);

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
