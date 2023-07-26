#include "MinicapStream.h"
#include "MinicapDef.h"

#include "Utils/Logger.hpp"
#include "Utils/Format.hpp"
#include "Utils/NoWarningCV.h"

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapStream::parse(const json::value& config)
{
    return MinicapBase::parse(config) && parse_argv("ForwardSocket", config, forward_argv_);
}

bool MinicapStream::init(int w, int h)
{
    LogFunc;

    if (!MinicapBase::init(w, h)) {
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
        return false;
    }

    while (true) {
        auto res = process_handle_->read(5);
        std::cout << res;
        if (res.find("Allocating") != std::string::npos) {
            break;
        }
    }

    auto serial_host = argv_replace_["{ADB_SERIAL}"];
    auto shp = serial_host.find(':');
    std::string local = "127.0.0.1";
    if (shp != std::string::npos) {
        local = serial_host.substr(0, shp);
    }

    // stream_handle_ = io_ptr_->tcp("172.27.176.1", 1313);
    stream_handle_ = io_ptr_->tcp(local, 1313);

    if (!stream_handle_) {
        return false;
    }

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

    return true;
}

std::optional<cv::Mat> MinicapStream::screencap()
{
    if (!stream_handle_) {
        return std::nullopt;
    }

    uint32_t size;
    if (!take_out(&size, 4)) {
        LogError << "take_out size failed";
        return std::nullopt;
    }

    if (!read_until(size)) {
        LogError << "read_until size failed";
        return std::nullopt;
    }

    auto data = std::move(buffer_);
    buffer_.clear();

    return screencap_helper_.process_data(
        data, std::bind(&ScreencapHelper::decode_jpg, &screencap_helper_, std::placeholders::_1));
}

bool MinicapStream::read_until(size_t size)
{
    LogFunc;

    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();

    while (buffer_.size() < size && duration_since(start) < 5s) {
        auto ret = stream_handle_->read(2, size - buffer_.size());
        buffer_.insert(buffer_.end(), ret.begin(), ret.end());
    }

    return buffer_.size() == size;
}

bool MinicapStream::take_out(void* out, size_t size)
{
    LogFunc;

    if (!read_until(size)) {
        return false;
    }
    if (out) {
        memcpy(out, buffer_.c_str(), size);
    }
    buffer_.erase(0, size);

    return true;
}

MAA_CTRL_UNIT_NS_END
