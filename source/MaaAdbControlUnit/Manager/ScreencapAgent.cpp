#include "ScreencapAgent.h"

#include <format>
#include <ranges>
#include <unordered_set>

#include "EmulatorExtras/MumuExternalRendererIpc.h"
#include "Screencap/Encode.h"
#include "Screencap/EncodeToFile.h"
#include "Screencap/Minicap/MinicapDirect.h"
#include "Screencap/Minicap/MinicapStream.h"
#include "Screencap/RawByNetcat.h"
#include "Screencap/RawWithGzip.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

ScreencapAgent::ScreencapAgent(MaaAdbScreencapMethod methods, const std::filesystem::path& agent_path)
{
    std::unordered_set<Method> method_set;
    if (methods & MaaAdbScreencapMethod_EncodeToFileAndPull) {
        method_set.emplace(ScreencapAgent::Method::EncodeToFileAndPull);
    }
    if (methods & MaaAdbScreencapMethod_Encode) {
        method_set.emplace(ScreencapAgent::Method::Encode);
    }
    if (methods & MaaAdbScreencapMethod_RawWithGzip) {
        method_set.emplace(ScreencapAgent::Method::RawWithGzip);
    }
    if (methods & MaaAdbScreencapMethod_RawByNetcat) {
        method_set.emplace(ScreencapAgent::Method::RawByNetcat);
    }
    if (methods & MaaAdbScreencapMethod_MinicapDirect) {
        method_set.emplace(ScreencapAgent::Method::MinicapDirect);
    }
    if (methods & MaaAdbScreencapMethod_MinicapStream) {
        method_set.emplace(ScreencapAgent::Method::MinicapStream);
    }
    if (methods & MaaAdbScreencapMethod_EmulatorExtras) {
        method_set.emplace(ScreencapAgent::Method::MumuExternalRendererIpc);
        // TODO: add LDPlayer and more...
    }

    LogInfo << VAR(methods) << VAR(method_set) << VAR(agent_path);

    for (Method method : method_set) {
        std::shared_ptr<ScreencapBase> unit = nullptr;
        switch (method) {
        case Method::RawByNetcat:
            unit = std::make_shared<ScreencapRawByNetcat>();
            break;
        case Method::RawWithGzip:
            unit = std::make_shared<ScreencapRawWithGzip>();
            break;
        case Method::Encode:
            unit = std::make_shared<ScreencapEncode>();
            break;
        case Method::EncodeToFileAndPull:
            unit = std::make_shared<ScreencapEncodeToFileAndPull>();
            break;
        case Method::MinicapDirect: {
            auto minicap_path = agent_path / "minicap";
            if (!std::filesystem::exists(minicap_path)) {
                LogWarn << "minicap path not exists" << VAR(minicap_path);
                break;
            }
            unit = std::make_shared<MinicapDirect>(minicap_path);
        } break;
        case Method::MinicapStream: {
            auto minicap_path = agent_path / "minicap";
            if (!std::filesystem::exists(minicap_path)) {
                LogWarn << "minicap path not exists" << VAR(minicap_path);
                break;
            }
            unit = std::make_shared<MinicapStream>(minicap_path);
        } break;
        case Method::MumuExternalRendererIpc:
            unit = std::make_shared<MumuExternalRendererIpc>();
            break;
        default:
            LogWarn << "Not support:" << method;
            break;
        }

        children_.emplace_back(unit);
        units_.emplace(method, unit);
    }
}

bool ScreencapAgent::parse(const json::value& config)
{
    bool ret = false;

    for (auto it = units_.begin(); it != units_.end();) {
        if (it->second->parse(config)) {
            ret = true; // 任一成功就行
            ++it;
        }
        else {
            LogWarn << "failed to parse" << it->first;
            it = units_.erase(it);
        }
    }

    return ret;
}

bool ScreencapAgent::init()
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

void ScreencapAgent::deinit()
{
    LogFunc;

    for (auto& unit : units_ | std::views::values) {
        unit->deinit();
    }

    method_ = Method::UnknownYet;
}

std::optional<cv::Mat> ScreencapAgent::screencap()
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

bool ScreencapAgent::speed_test()
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
    static const std::unordered_set<Method> kDropFirst = { Method::RawByNetcat, Method::MinicapStream };

    for (auto& [method, unit] : units_) {
        if (kDropFirst.contains(method)) {
            LogInfo << "Testing" << method << "drop first";
            if (!unit->screencap()) {
                LogWarn << "failed to test" << method;
                continue;
            }
        }

        LogInfo << "Testing" << method;
        auto now = std::chrono::steady_clock::now();
        if (!unit->screencap()) {
            LogWarn << "failed to test" << method;
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

std::ostream& operator<<(std::ostream& os, ScreencapAgent::Method m)
{
    switch (m) {
    case ScreencapAgent::Method::UnknownYet:
        os << "UnknownYet";
        break;
    case ScreencapAgent::Method::RawByNetcat:
        os << "RawByNetcat";
        break;
    case ScreencapAgent::Method::RawWithGzip:
        os << "RawWithGzip";
        break;
    case ScreencapAgent::Method::Encode:
        os << "Encode";
        break;
    case ScreencapAgent::Method::EncodeToFileAndPull:
        os << "EncodeToFileAndPull";
        break;
    case ScreencapAgent::Method::MinicapDirect:
        os << "MinicapDirect";
        break;
    case ScreencapAgent::Method::MinicapStream:
        os << "MinicapStream";
        break;
    case ScreencapAgent::Method::MumuExternalRendererIpc:
        os << "MumuExternalRendererIpc";
        break;
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
