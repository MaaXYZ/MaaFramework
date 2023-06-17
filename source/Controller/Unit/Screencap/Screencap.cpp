#include "Screencap.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

MAA_CTRL_UNIT_NS_BEGIN

Screencap::Screencap()
{
    children_.emplace_back(raw_by_netcat_uint_);
    children_.emplace_back(raw_with_gzip_unit_);
    children_.emplace_back(encode_unit_);
    children_.emplace_back(encode_to_file_unit_);
}

bool Screencap::parse(const json::value& config)
{
    bool ret = raw_by_netcat_uint_->parse(config);
    ret |= raw_with_gzip_unit_->parse(config);
    ret |= encode_unit_->parse(config);
    ret |= encode_to_file_unit_->parse(config);
    return ret;
}

bool Screencap::init(int w, int h, const std::string& force_temp)
{
    LogFunc;

    raw_by_netcat_uint_->init(w, h);
    raw_with_gzip_unit_->init(w, h);
    encode_unit_->init(w, h);
    encode_to_file_unit_->init(w, h, force_temp);

    return speed_test();
}

void Screencap::deinit()
{
    raw_by_netcat_uint_->deinit();
    raw_with_gzip_unit_->deinit();
    encode_unit_->deinit();
    encode_to_file_unit_->deinit();

    method_ = Method::UnknownYet;
}

std::optional<cv::Mat> Screencap::screencap()
{
    switch (method_) {
    case Method::UnknownYet:
        LogError << "Unknown screencap method";
        return std::nullopt;
    case Method::RawByNetcat:
        return raw_by_netcat_uint_->screencap();
    case Method::RawWithGzip:
        return raw_with_gzip_unit_->screencap();
    case Method::Encode:
        return encode_unit_->screencap();
    case Method::EncodeToFileAndPull:
        return encode_to_file_unit_->screencap();
    default:
        LogInfo << "Not support:" << method_;
        break;
    }
    return std::nullopt;
}

bool Screencap::speed_test()
{
    LogFunc;

    method_ = Method::UnknownYet;
    std::chrono::nanoseconds cost(INT64_MAX);

    auto check = [this, &cost](Method method, std::chrono::steady_clock::time_point start) {
        auto duration = std::chrono::steady_clock::now() - start;
        if (duration < cost) {
            method_ = method;
            cost = duration;
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        LogInfo << VAR(method) << VAR(ms);
    };

    {
        auto now = std::chrono::steady_clock::now();
        if (raw_by_netcat_uint_->screencap()) {
            check(Method::RawByNetcat, now);
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        if (raw_with_gzip_unit_->screencap()) {
            check(Method::RawWithGzip, now);
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        if (encode_unit_->screencap()) {
            check(Method::Encode, now);
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        if (encode_to_file_unit_->screencap()) {
            check(Method::EncodeToFileAndPull, now);
        }
    }

    if (method_ == Method::UnknownYet) {
        LogError << "cannot find any method to screencap!";
        return false;
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(cost);
    LogInfo << "The fastest method is " << method_ << VAR(ms);
    return true;
}

std::ostream& operator<<(std::ostream& os, Screencap::Method m)
{
    switch (m) {
    case Screencap::Method::UnknownYet:
        os << "UnknownYet";
        break;
    case Screencap::Method::RawByNetcat:
        os << "RawByNetcat";
        break;
    case Screencap::Method::RawWithGzip:
        os << "RawWithGzip";
        break;
    case Screencap::Method::Encode:
        os << "Encode";
        break;
    case Screencap::Method::EncodeToFileAndPull:
        os << "EncodeToFileAndPull";
        break;
    case Screencap::Method::MinicapDirect:
        os << "MinicapDirect";
        break;
    case Screencap::Method::MinicapStream:
        os << "MinicapStream";
        break;
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
