#include "NativeScreencap.h"

#include <android/bitmap.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

NativeScreencap::~NativeScreencap()
{
    LogFunc;

    if (screencap_helper_ && jvm_) {
        JNIEnv* env = nullptr;
        if (jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK && env) {
            env->DeleteGlobalRef(screencap_helper_);
        }
    }
}

bool NativeScreencap::init()
{
    LogFunc;

    if (!init_display_info()) {
        LogError << "failed to init display info";
        return false;
    }

    return true;
}

bool NativeScreencap::init_display_info()
{
    LogFunc;

    // 通过 JNI 获取屏幕分辨率
    // 这需要外部传入 JNIEnv 或者通过其他方式获取
    // 暂时使用默认值，实际使用时需要从 Java 层获取

    // 获取当前线程的 JNIEnv
    JNIEnv* env = nullptr;

    if (!jvm_) {
        // 尝试获取 JavaVM
        // 这通常在 JNI_OnLoad 中保存
        LogWarn << "JavaVM not set, using default resolution";
        display_width_ = 1920;
        display_height_ = 1080;
        return true;
    }

    int status = jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        if (jvm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LogError << "failed to attach current thread";
            return false;
        }
    }
    else if (status != JNI_OK) {
        LogError << "failed to get JNIEnv";
        return false;
    }

    // 获取 DisplayMetrics
    jclass display_metrics_class = env->FindClass("android/util/DisplayMetrics");
    if (!display_metrics_class) {
        LogError << "failed to find DisplayMetrics class";
        return false;
    }

    // 这里需要通过 Context 获取 WindowManager，然后获取 DisplayMetrics
    // 简化处理，假设已经有分辨率信息
    LogWarn << "Using default resolution, actual implementation needs Context";
    display_width_ = 1920;
    display_height_ = 1080;

    return true;
}

bool NativeScreencap::screencap(cv::Mat& image)
{
    return screencap_screenshot_api(image);
}

bool NativeScreencap::screencap_screenshot_api(cv::Mat& image)
{
    LogFunc;

    if (!jvm_) {
        LogError << "JavaVM not set";
        return false;
    }

    JNIEnv* env = nullptr;
    bool need_detach = false;

    int status = jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        if (jvm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LogError << "failed to attach current thread";
            return false;
        }
        need_detach = true;
    }
    else if (status != JNI_OK) {
        LogError << "failed to get JNIEnv";
        return false;
    }

    if (!screencap_helper_ || !take_screenshot_method_) {
        LogError << "screencap helper not initialized";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    // 调用 Java 方法获取截图
    jobject bitmap = env->CallObjectMethod(screencap_helper_, take_screenshot_method_);
    if (!bitmap) {
        LogError << "failed to take screenshot";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    // 将 Bitmap 转换为 cv::Mat
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS) {
        LogError << "failed to get bitmap info";
        env->DeleteLocalRef(bitmap);
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    void* pixels = nullptr;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) {
        LogError << "failed to lock bitmap pixels";
        env->DeleteLocalRef(bitmap);
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    // 根据 Bitmap 格式创建 cv::Mat
    cv::Mat tmp;
    switch (info.format) {
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
        tmp = cv::Mat(info.height, info.width, CV_8UC4, pixels);
        cv::cvtColor(tmp, image, cv::COLOR_RGBA2BGR);
        break;
    case ANDROID_BITMAP_FORMAT_RGB_565:
        tmp = cv::Mat(info.height, info.width, CV_8UC2, pixels);
        cv::cvtColor(tmp, image, cv::COLOR_BGR5652BGR);
        break;
    default:
        LogError << "unsupported bitmap format:" << info.format;
        AndroidBitmap_unlockPixels(env, bitmap);
        env->DeleteLocalRef(bitmap);
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    env->DeleteLocalRef(bitmap);

    if (need_detach) {
        jvm_->DetachCurrentThread();
    }

    return !image.empty();
}

std::pair<int, int> NativeScreencap::get_resolution() const
{
    return { display_width_, display_height_ };
}

MAA_CTRL_UNIT_NS_END
