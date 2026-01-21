#ifdef _WIN32

#include "LDPlayerExtras.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

LDPlayerExtras::~LDPlayerExtras()
{
    release_ld_instance();
}

bool LDPlayerExtras::parse(const json::value& config)
{
    bool enable = config.get("extras", "ld", "enable", false);
    if (!enable) {
        LogInfo << "extras.ld.enable is false, ignore";
        return false;
    }

    std::string lib = config.get("extras", "ld", "lib", "");
    if (lib.empty()) {
        std::string p = config.get("extras", "ld", "path", "");
        lib_path_ = MAA_NS::path(p) / MAA_NS::path("ldopengl64");
    }
    else {
        lib_path_ = MAA_NS::path(lib);
    }

    ld_index_ = config.get("extras", "ld", "index", 0);
    ld_pid_ = config.get("extras", "ld", "pid", 0);

    bool info_parsed = device_info_->parse(config);
    LogInfo << VAR(lib_path_) << VAR(ld_index_) << VAR(ld_pid_) << VAR(info_parsed);
    return info_parsed;
}

bool LDPlayerExtras::init()
{
    return load_ld_library() && create_ld_instance();
}

std::optional<cv::Mat> LDPlayerExtras::screencap()
{
    if (!ld_handle_) {
        LogError << "ld_handle_ is null";
        return std::nullopt;
    }

    void* data = ld_handle_->cap();
    if (!data) {
        LogError << "Failed to cap, ret is null" << VAR(ld_handle_) << VAR(ld_index_) << VAR(ld_pid_);
        return std::nullopt;
    }

    cv::Mat raw(display_height_, display_width_, CV_8UC3, data);
    cv::Mat dst;
    cv::flip(raw, dst, 0);

    return dst;
}

bool LDPlayerExtras::load_ld_library()
{
    if (!load_library(lib_path_)) {
        LogError << "Failed to load library" << VAR(lib_path_);
        return false;
    }

    create_instance_func_ = get_function<decltype(dnopengl::CreateScreenShotInstance)>(kCreateInstanceFuncName);
    if (!create_instance_func_) {
        LogError << "Failed to get function" << VAR(kCreateInstanceFuncName);
        return false;
    }

    auto resolution_opt = device_info_->request_resolution();
    if (!resolution_opt) {
        LogError << "Failed to get display resolution";
        return false;
    }

    display_width_ = resolution_opt->w;
    display_height_ = resolution_opt->h;
    LogInfo << "Display resolution: " << display_width_ << "x" << display_height_;

    return true;
}

bool LDPlayerExtras::create_ld_instance()
{
    LogFunc << VAR(ld_index_) << VAR(ld_pid_);

    if (!create_instance_func_) {
        LogError << "create_instance_func_ is null";
        return false;
    }

    if (ld_handle_) {
        LogWarn << "ld_handle_ is not null";
        release_ld_instance();
    }

    ld_handle_ = create_instance_func_(ld_index_, ld_pid_);
    LogInfo << VAR_VOIDP(ld_handle_);

    if (!ld_handle_) {
        LogError << "Failed to create ld inst" << VAR(ld_index_) << VAR(ld_pid_);
        return false;
    }

    return true;
}

void LDPlayerExtras::release_ld_instance()
{
    LogFunc << VAR_VOIDP(ld_handle_);

    if (ld_handle_) {
        ld_handle_->release();
    }
    ld_handle_ = nullptr;
}

MAA_CTRL_UNIT_NS_END

#endif
