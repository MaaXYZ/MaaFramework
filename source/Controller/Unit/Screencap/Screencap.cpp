#include "Screencap.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

#include <format>

MAA_CTRL_UNIT_NS_BEGIN

Screencap::Screencap()
    : units_({ std::make_shared<ScreencapRawByNetcat>(), std::make_shared<ScreencapRawWithGzip>(),
               std::make_shared<ScreencapEncode>(), std::make_shared<ScreencapEncodeToFileAndPull>(),
               std::make_shared<MinicapDirect>(), std::make_shared<MinicapStream>() })
{
    children_.reserve(units_.size());
    for (auto unit : units_) {
        children_.push_back(unit);
    }
}

bool Screencap::parse(const json::value& config)
{
    bool ret = false;
    for (auto unit : units_) {
        // TODO: 也许可以考虑删除无法初始化的unit
        ret |= unit->parse(config);
    }
    return ret;
}

bool Screencap::init(int w, int h)
{
    LogFunc;

    for (auto unit : units_) {
        unit->init(w, h);
    }

    return speed_test();
}

void Screencap::deinit()
{
    LogFunc;

    for (auto unit : units_) {
        unit->deinit();
    }

    method_ = Method::UnknownYet;
}

std::optional<cv::Mat> Screencap::screencap()
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
        return units_[int(method_) - 1]->screencap();
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

    for (size_t i = 0; i < units_.size(); i++) {
        auto now = std::chrono::steady_clock::now();
        if (units_[i]->screencap()) {
            check(Method(i + 1), now);
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
