#include "MinicapStream.h"
#include "Minicap.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

#include <format>

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapStream::parse(const json::value& config)
{
    return MinicapBase::parse(config) && parse_argv("ForwardSocket", config, forward_argv_);
}

bool MinicapStream::init(int w, int h, std::function<std::string(const std::string&)> path_of_bin,
                         std::function<std::string(const std::string&, int)> path_of_lib, const std::string& force_temp)
{
    LogFunc;

    // if (!MinicapBase::init(w, h, path_of_bin, path_of_lib, "minicap")) {
    if (!MinicapBase::init(w, h, path_of_bin, path_of_lib, force_temp)) {
        return false;
    }

    // TODO: 也许可以允许配置?
    merge_replacement({ { "{FOWARD_PORT}", "1313" }, { "{LOCAL_SOCKET}", "minicap" } });
    auto cmd_ret = command(forward_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return false;
    }

    process_handle_ = binary_->invoke_bin(std::format("-P {}x{}@{}x{}/{}", width_, height_, width_, height_, 0));

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

    take_out(&header, sizeof(header));

    LogInfo << header.version << header.size << header.pid << header.realWidth << header.realHeight << header.virtWidth
            << header.virtHeight << header.orientation << header.flags;

    if (header.version != 1 || header.size < sizeof(header)) {
        return false;
    }

    if (header.realWidth != width_ || header.realHeight != height_ || header.virtWidth != width_ ||
        header.virtHeight != height_) {
        return false;
    }

    take_out(nullptr, header.size - sizeof(header));

    return true;
}

std::optional<cv::Mat> MinicapStream::screencap()
{
    LogFunc;

    if (!stream_handle_) {
        return std::nullopt;
    }

    uint32_t size;
    take_out(&size, 4);

    read_until(size);

    auto data = std::move(buffer_);
    buffer_.clear();

    return process_data(data, std::bind(&ScreencapBase::decode_jpg, this, std::placeholders::_1));
}

void MinicapStream::read_until(size_t size)
{
    // TODO: 出问题了记得退出
    while (buffer_.size() < size) {
        auto ret = stream_handle_->read(2, size - buffer_.size());
        buffer_.insert(buffer_.end(), ret.begin(), ret.end());
    }
}

void MinicapStream::take_out(void* out, size_t size)
{
    read_until(size);
    if (out) {
        memcpy(out, buffer_.c_str(), size);
    }
    buffer_.erase(buffer_.begin(), buffer_.begin() + size);
}

MAA_CTRL_UNIT_NS_END
