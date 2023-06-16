#include "Screencap.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

MAA_CTRL_UNIT_NS_BEGIN

bool Screencap::parse(const json::value& config)
{
    return screencap_raw_by_netcat_uint_.parse(config) || screencap_raw_with_gzip_unit_.parse(config) ||
           screencap_encode_unit_.parse(config) || screencap_encode_to_file_unit_.parse(config);
}

bool Screencap::init(int w, int h, const std::string& force_temp)
{
    LogFunc;

    screencap_raw_by_netcat_uint_.set_io(io_ptr_);
    screencap_raw_with_gzip_unit_.set_io(io_ptr_);
    screencap_encode_unit_.set_io(io_ptr_);
    screencap_encode_to_file_unit_.set_io(io_ptr_);

    screencap_raw_by_netcat_uint_.init(w, h);
    screencap_raw_with_gzip_unit_.init(w, h);
    screencap_encode_unit_.init(w, h);
    screencap_encode_to_file_unit_.init(w, h, force_temp);

    screencap_raw_by_netcat_uint_.set_replacement(argv_replace_);
    screencap_raw_with_gzip_unit_.set_replacement(argv_replace_);
    screencap_encode_unit_.set_replacement(argv_replace_);
    screencap_encode_to_file_unit_.set_replacement(argv_replace_);

    return speed_test();
}

void Screencap::deinit()
{
    screencap_raw_by_netcat_uint_.deinit();
    screencap_raw_with_gzip_unit_.deinit();
    screencap_encode_unit_.deinit();
    screencap_encode_to_file_unit_.deinit();

    method_ = Method::UnknownYet;
}

std::optional<cv::Mat> Screencap::screencap()
{
    switch (method_) {
    case Method::UnknownYet:
        LogError << "Unknown screencap method";
        return std::nullopt;
    case Method::RawByNetcat:
        return screencap_raw_by_netcat_uint_.screencap();
    case Method::RawWithGzip:
        return screencap_raw_with_gzip_unit_.screencap();
    case Method::Encode:
        return screencap_encode_unit_.screencap();
    case Method::EncodeToFileAndPull:
        return screencap_encode_to_file_unit_.screencap();
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
        LogInfo << VAR(method) << VAR(duration);
    };

    {
        auto now = std::chrono::steady_clock::now();
        if (screencap_raw_by_netcat_uint_.screencap()) {
            check(Method::RawByNetcat, now);
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        if (screencap_raw_with_gzip_unit_.screencap()) {
            check(Method::RawWithGzip, now);
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        if (screencap_encode_unit_.screencap()) {
            check(Method::Encode, now);
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        if (screencap_encode_to_file_unit_.screencap()) {
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
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
