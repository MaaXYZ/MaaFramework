#pragma once

#include <jni.h>

#include "Base/ScreencapBase.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Conf/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// 使用 Android ScreenCapture API (需要 MediaProjection 权限) 或 SurfaceControl (需要系统权限)
class NativeScreencap : public ScreencapBase
{
public:
    NativeScreencap() = default;
    virtual ~NativeScreencap() override;

public:
    virtual bool init() override;
    virtual bool screencap(cv::Mat& image) override;
    virtual std::pair<int, int> get_resolution() const override;

private:
    bool init_display_info();
    bool screencap_screenshot_api(cv::Mat& image);

    int display_width_ = 0;
    int display_height_ = 0;

    // JNI 相关
    JavaVM* jvm_ = nullptr;
    jobject screencap_helper_ = nullptr;
    jmethodID take_screenshot_method_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
