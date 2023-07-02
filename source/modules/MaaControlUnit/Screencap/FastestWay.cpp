#include "FastestWay.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/NoWarningCV.h"

#include <format>

MAA_CTRL_UNIT_NS_BEGIN

ScreencapFastestWay::ScreencapFastestWay()
{
    units_ = {
        { Method::RawByNetcat, std::make_shared<ScreencapRawByNetcat>() },
        { Method::RawWithGzip, std::make_shared<ScreencapRawWithGzip>() },
        { Method::Encode, std::make_shared<ScreencapEncode>() },
        { Method::EncodeToFileAndPull, std::make_shared<ScreencapEncodeToFileAndPull>() },
        { Method::MinicapDirect, std::make_shared<MinicapDirect>() },
        { Method::MinicapStream, std::make_shared<MinicapStream>() },
    };

    for (auto pair : units_) {
        children_.emplace_back(pair.second);
    }
}

bool ScreencapFastestWay::parse(const json::value& config)
{
    bool ret = false;
    for (auto pair : units_) {
        // TODO: 也许可以考虑删除无法初始化的unit
        ret |= pair.second->parse(config);
    }
    return ret;
}

bool ScreencapFastestWay::init(int w, int h)
{
    LogFunc;

    for (auto pair : units_) {
        pair.second->init(w, h);
    }

    return speed_test();
}

void ScreencapFastestWay::deinit()
{
    LogFunc;

    for (auto pair : units_) {
        pair.second->deinit();
    }

    method_ = Method::UnknownYet;
}

std::optional<cv::Mat> ScreencapFastestWay::screencap()
{
    switch (method_) {
    case Method::UnknownYet:
        LogError << "Unknown screencap method";
        return std::nullopt;
    case Method::RawByNetcat:
    case Method::RawWithGzip:
    case Method::Encode:
    case Method::EncodeToFileAndPull:
    case Method::MinicapDirect:
    case Method::MinicapStream:
        return units_[method_]->screencap();
    default:
        LogInfo << "Not support:" << method_;
        break;
    }
    return std::nullopt;
}

bool ScreencapFastestWay::speed_test()
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

    for (auto pair : units_) {
        auto now = std::chrono::steady_clock::now();
        if (pair.second->screencap()) {
            check(pair.first, now);
        }
    }

    if (method_ == Method::UnknownYet) {
        LogError << "cannot find any method to screencap!";
        return false;
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(cost);
    LogInfo << "The fastest method is" << method_ << VAR(ms);
    return true;
}

std::ostream& operator<<(std::ostream& os, ScreencapFastestWay::Method m)
{
    switch (m) {
    case ScreencapFastestWay::Method::UnknownYet:
        os << "UnknownYet";
        break;
    case ScreencapFastestWay::Method::RawByNetcat:
        os << "RawByNetcat";
        break;
    case ScreencapFastestWay::Method::RawWithGzip:
        os << "RawWithGzip";
        break;
    case ScreencapFastestWay::Method::Encode:
        os << "Encode";
        break;
    case ScreencapFastestWay::Method::EncodeToFileAndPull:
        os << "EncodeToFileAndPull";
        break;
    case ScreencapFastestWay::Method::MinicapDirect:
        os << "MinicapDirect";
        break;
    case ScreencapFastestWay::Method::MinicapStream:
        os << "MinicapStream";
        break;
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
