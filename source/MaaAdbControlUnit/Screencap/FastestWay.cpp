#include "FastestWay.h"

#include <format>
#include <ranges>
#include <unordered_set>

#include "EmulatorExtras/MumuExternalRendererIpc.h"
#include "Encode.h"
#include "EncodeToFile.h"
#include "Minicap/MinicapDirect.h"
#include "Minicap/MinicapStream.h"
#include "RawByNetcat.h"
#include "RawWithGzip.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

ScreencapFastestWay::ScreencapFastestWay(const std::filesystem::path& minicap_path, bool lossless)
{
    units_ = {
#ifdef _WIN32
        { Method::RawByNetcat, std::make_shared<ScreencapRawByNetcat>() },
#endif
        { Method::RawWithGzip, std::make_shared<ScreencapRawWithGzip>() },
        { Method::Encode, std::make_shared<ScreencapEncode>() },
        { Method::EncodeToFileAndPull, std::make_shared<ScreencapEncodeToFileAndPull>() },
        { Method::MumuExternalRendererIpc, std::make_shared<MumuExternalRendererIpc>() },
    };

    if (!lossless) {
        if (std::filesystem::exists(minicap_path)) {
            units_.merge(decltype(units_) {
                { Method::MinicapDirect, std::make_shared<MinicapDirect>(minicap_path) },
                { Method::MinicapStream, std::make_shared<MinicapStream>(minicap_path) },
            });
        }
        else {
            LogWarn << "minicap path not exists" << VAR(minicap_path);
        }
    }

    for (auto& unit : units_ | std::views::values) {
        children_.emplace_back(unit);
    }
}

bool ScreencapFastestWay::parse(const json::value& config)
{
    bool ret = false;

    for (auto it = units_.begin(); it != units_.end();) {
        if (it->second->parse(config)) {
            ret = true; // 任一成功就行
            ++it;
        }
        else {
            it = units_.erase(it);
        }
    }

    return ret;
}

bool ScreencapFastestWay::init()
{
    LogFunc;

    for (auto it = units_.begin(); it != units_.end();) {
        if (it->second->init()) {
            ++it;
        }
        else {
            it = units_.erase(it);
        }
    }

    return speed_test();
}

void ScreencapFastestWay::deinit()
{
    LogFunc;

    for (auto& unit : units_ | std::views::values) {
        unit->deinit();
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
    case Method::MumuExternalRendererIpc:
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
    std::chrono::milliseconds cost(INT64_MAX);

    auto check = [this, &cost](Method method, std::chrono::steady_clock::time_point start) {
        auto duration = duration_since(start);
        if (duration < cost) {
            method_ = method;
            cost = duration;
        }
        LogInfo << VAR(method) << VAR(duration);
    };

    // RawByNetcat 第一次速度很慢，但后面快
    // MinicapStream 是从缓存拉数据，只取一次不准
    static const std::unordered_set<Method> kDropFirst = { Method::RawByNetcat,
                                                           Method::MinicapStream };

    for (auto& [method, unit] : units_) {
        if (kDropFirst.contains(method)) {
            LogInfo << "Testing" << method << "drop first";
            if (!unit->screencap()) {
                LogWarn << "failed to test";
                continue;
            }
        }

        LogInfo << "Testing" << method;
        auto now = std::chrono::steady_clock::now();
        if (!unit->screencap()) {
            LogWarn << "failed to test";
            continue;
        }
        check(method, now);
    }

    if (method_ == Method::UnknownYet) {
        LogError << "cannot find any method to screencap!";
        return false;
    }

    LogInfo << "The fastest method is" << method_ << VAR(cost);
    for (auto& [method, unit] : units_) {
        if (method == method_) {
            continue;
        }
        unit->deinit();
    }

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
    case ScreencapFastestWay::Method::MumuExternalRendererIpc:
        os << "MumuExternalRendererIpc";
        break;
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
