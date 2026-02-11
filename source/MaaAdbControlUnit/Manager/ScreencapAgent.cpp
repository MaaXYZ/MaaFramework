#include "ScreencapAgent.h"

#include <format>
#include <ranges>
#include <unordered_set>

#include "EmulatorExtras/AVDExtras.h"
#include "EmulatorExtras/LDPlayerExtras.h"
#include "EmulatorExtras/MuMuPlayerExtras.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Screencap/Encode.h"
#include "Screencap/EncodeToFile.h"
#include "Screencap/Minicap/MinicapDirect.h"
#include "Screencap/Minicap/MinicapStream.h"
#include "Screencap/RawByNetcat.h"
#include "Screencap/RawWithGzip.h"

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
#ifdef _WIN32
        method_set.emplace(ScreencapAgent::Method::MuMuPlayerExtras);
        method_set.emplace(ScreencapAgent::Method::LDPlayerExtras);
#elif !defined(__ANDROID__)
        method_set.emplace(ScreencapAgent::Method::AVDExtras);
#else
        LogWarn << "EmulatorExtras is not supported on this platform";
#endif
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

#ifdef _WIN32
        case Method::MuMuPlayerExtras:
            unit = std::make_shared<MuMuPlayerExtras>();
            break;
        case Method::LDPlayerExtras:
            unit = std::make_shared<LDPlayerExtras>();
            break;
#endif
#ifndef __ANDROID__
        case Method::AVDExtras:
            unit = std::make_shared<AVDExtras>();
            break;
#endif

        default:
            LogWarn << "Not support:" << method;
            break;
        }

        if (!unit) {
            LogWarn << "failed to create" << method;
            continue;
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

    if (active_unit_) {
        LogError << "already initialized" << VAR(active_unit_);
        return false;
    }

    for (auto it = units_.begin(); it != units_.end();) {
        if (it->second->init()) {
            ++it;
        }
        else {
            it = units_.erase(it);
        }
    }

    active_unit_ = speed_test();
    if (!active_unit_) {
        LogError << "No available screencap method";
        return false;
    }

    units_.clear();
    return true;
}

std::optional<cv::Mat> ScreencapAgent::screencap()
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return std::nullopt;
    }

    return active_unit_->screencap();
}

void ScreencapAgent::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return;
    }

    active_unit_->on_image_resolution_changed(pre, cur);
}

void ScreencapAgent::on_app_started(const std::string& intent)
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return;
    }

    active_unit_->on_app_started(intent);
}

void ScreencapAgent::on_app_stopped(const std::string& intent)
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return;
    }

    active_unit_->on_app_stopped(intent);
}

std::shared_ptr<ScreencapBase> ScreencapAgent::speed_test()
{
    LogFunc;

    Method fastest = Method::UnknownYet;
    std::chrono::milliseconds cost(INT64_MAX);

    auto check = [&fastest, &cost](Method method, std::chrono::steady_clock::time_point start) {
        auto duration = duration_since(start);
        if (duration < cost) {
            fastest = method;
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

    if (fastest == Method::UnknownYet) {
        LogError << "cannot find any method to screencap!";
        return nullptr;
    }

    LogInfo << "The fastest method is" << fastest << VAR(cost);
    return units_[fastest];
}

MAA_CTRL_UNIT_NS_END
