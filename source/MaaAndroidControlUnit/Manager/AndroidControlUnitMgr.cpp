#include "AndroidControlUnitMgr.h"

#include <android/bitmap.h>
#include <cstring>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{
JavaVM* g_vm = nullptr;
}

AndroidControlUnitMgr::AndroidControlUnitMgr(MaaAndroidScreencapMethod screencap_methods, MaaAndroidInputMethod input_methods)
    : screencap_methods_(screencap_methods)
    , input_methods_(input_methods)
    , vm_(g_vm)
{
}

JNIEnv* AndroidControlUnitMgr::ensure_env()
{
    if (!vm_) {
        vm_ = g_vm;
        if (!vm_) {
            return nullptr;
        }
    }
    JNIEnv* env = nullptr;
    if (vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        return env;
    }
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        return nullptr;
    }
    return env;
}

jclass AndroidControlUnitMgr::bridge_class()
{
    if (bridge_cls_) {
        return bridge_cls_;
    }
    JNIEnv* env = ensure_env();
    if (!env) {
        return nullptr;
    }
    jclass local = env->FindClass("com/maa/framework/nativectrl/NativeBridge");
    if (!local) {
        LogError << "NativeBridge class not found";
        return nullptr;
    }
    bridge_cls_ = reinterpret_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);
    return bridge_cls_;
}

template <typename Callable>
bool AndroidControlUnitMgr::call_bool(const char* name, const char* sig, Callable&& caller)
{
    JNIEnv* env = ensure_env();
    if (!env) {
        LogError << "JNIEnv null";
        return false;
    }
    jclass cls = bridge_class();
    if (!cls) {
        return false;
    }
    jmethodID mid = env->GetStaticMethodID(cls, name, sig);
    if (!mid) {
        LogError << "Method not found" << name;
        return false;
    }
    jboolean ok = caller(env, cls, mid);
    return ok == JNI_TRUE;
}

bool AndroidControlUnitMgr::connect()
{
    // First call init to pass screencap_methods and input_methods to Kotlin side
    JNIEnv* env = ensure_env();
    if (!env) {
        LogError << "JNIEnv null";
        return false;
    }
    jclass cls = bridge_class();
    if (!cls) {
        return false;
    }

    // Call init(screencapMethods, inputMethods)
    jmethodID init_mid = env->GetStaticMethodID(cls, "init", "(JJ)Z");
    if (!init_mid) {
        LogError << "Method init not found";
        return false;
    }
    jboolean init_ok =
        env->CallStaticBooleanMethod(cls, init_mid, static_cast<jlong>(screencap_methods_), static_cast<jlong>(input_methods_));
    if (!init_ok) {
        LogError << "init failed";
        return false;
    }

    // Call connect()
    return call_bool("connect", "()Z", [](JNIEnv* env, jclass cls, jmethodID mid) { return env->CallStaticBooleanMethod(cls, mid); });
}

bool AndroidControlUnitMgr::request_uuid(/*out*/ std::string& uuid)
{
    JNIEnv* env = ensure_env();
    if (!env) {
        LogError << "JNIEnv null";
        return false;
    }
    jclass cls = bridge_class();
    if (!cls) {
        return false;
    }
    jmethodID mid = env->GetStaticMethodID(cls, "requestUuid", "()Ljava/lang/String;");
    if (!mid) {
        LogError << "Method requestUuid not found";
        return false;
    }
    auto str = static_cast<jstring>(env->CallStaticObjectMethod(cls, mid));
    if (!str) {
        return false;
    }
    const char* chars = env->GetStringUTFChars(str, nullptr);
    uuid = chars ? chars : "";
    if (chars) {
        env->ReleaseStringUTFChars(str, chars);
    }
    env->DeleteLocalRef(str);
    return !uuid.empty();
}

MaaControllerFeature AndroidControlUnitMgr::get_features() const
{
    return MaaControllerFeature_None;
}

bool AndroidControlUnitMgr::start_app(const std::string& intent)
{
    return call_bool("startApp", "(Ljava/lang/String;)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        jstring jintent = env->NewStringUTF(intent.c_str());
        jboolean ok = env->CallStaticBooleanMethod(cls, mid, jintent);
        env->DeleteLocalRef(jintent);
        return ok;
    });
}

bool AndroidControlUnitMgr::stop_app(const std::string& intent)
{
    return call_bool("stopApp", "(Ljava/lang/String;)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        jstring jintent = env->NewStringUTF(intent.c_str());
        jboolean ok = env->CallStaticBooleanMethod(cls, mid, jintent);
        env->DeleteLocalRef(jintent);
        return ok;
    });
}

bool AndroidControlUnitMgr::screencap(/*out*/ cv::Mat& image)
{
    JNIEnv* env = ensure_env();
    if (!env) {
        LogError << "JNIEnv null";
        return false;
    }
    jclass cls = bridge_class();
    if (!cls) {
        return false;
    }
    jmethodID mid = env->GetStaticMethodID(cls, "screencap", "()Landroid/graphics/Bitmap;");
    if (!mid) {
        LogError << "Method screencap not found";
        return false;
    }
    jobject bmp = env->CallStaticObjectMethod(cls, mid);
    if (!bmp) {
        return false;
    }

    AndroidBitmapInfo info {};
    if (AndroidBitmap_getInfo(env, bmp, &info) != ANDROID_BITMAP_RESULT_SUCCESS) {
        env->DeleteLocalRef(bmp);
        return false;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 && info.format != ANDROID_BITMAP_FORMAT_RGB_565) {
        LogError << "Unsupported bitmap format" << info.format;
        env->DeleteLocalRef(bmp);
        return false;
    }

    void* pixels = nullptr;
    if (AndroidBitmap_lockPixels(env, bmp, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS || !pixels) {
        env->DeleteLocalRef(bmp);
        return false;
    }

    const int cv_type = CV_8UC4;
    image.create(static_cast<int>(info.height), static_cast<int>(info.width), cv_type);

    if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        const size_t row_bytes = info.stride;
        for (uint32_t row = 0; row < info.height; ++row) {
            std::memcpy(image.ptr(row), static_cast<uint8_t*>(pixels) + row * row_bytes, image.cols * 4);
        }
    }
    else {
        // RGB565 -> RGBA8888 expansion
        for (uint32_t row = 0; row < info.height; ++row) {
            const uint16_t* src = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pixels) + row * info.stride);
            uint8_t* dst = image.ptr(row);
            for (uint32_t col = 0; col < info.width; ++col) {
                uint16_t v = src[col];
                uint8_t r = ((v >> 11) & 0x1F) << 3;
                uint8_t g = ((v >> 5) & 0x3F) << 2;
                uint8_t b = (v & 0x1F) << 3;
                dst[col * 4 + 0] = b;
                dst[col * 4 + 1] = g;
                dst[col * 4 + 2] = r;
                dst[col * 4 + 3] = 255;
            }
        }
    }

    AndroidBitmap_unlockPixels(env, bmp);
    env->DeleteLocalRef(bmp);
    return true;
}

bool AndroidControlUnitMgr::click(int x, int y)
{
    return call_bool("tap", "(II)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) { return env->CallStaticBooleanMethod(cls, mid, x, y); });
}

bool AndroidControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    return call_bool("swipe", "(IIIII)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        return env->CallStaticBooleanMethod(cls, mid, x1, y1, x2, y2, duration);
    });
}

bool AndroidControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = pressure;
    return click(x, y);
}

bool AndroidControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = pressure;
    std::ignore = x;
    std::ignore = y;
    return true;
}

bool AndroidControlUnitMgr::touch_up(int contact)
{
    std::ignore = contact;
    return true;
}

bool AndroidControlUnitMgr::click_key(int key)
{
    return call_bool("keyEvent", "(IZ)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        return env->CallStaticBooleanMethod(cls, mid, key, static_cast<jboolean>(true));
    });
}

bool AndroidControlUnitMgr::input_text(const std::string& text)
{
    return call_bool("inputText", "(Ljava/lang/String;)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        jstring jtext = env->NewStringUTF(text.c_str());
        jboolean ok = env->CallStaticBooleanMethod(cls, mid, jtext);
        env->DeleteLocalRef(jtext);
        return ok;
    });
}

bool AndroidControlUnitMgr::key_down(int key)
{
    return call_bool("keyEvent", "(IZ)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        return env->CallStaticBooleanMethod(cls, mid, key, static_cast<jboolean>(true));
    });
}

bool AndroidControlUnitMgr::key_up(int key)
{
    return call_bool("keyEvent", "(IZ)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        return env->CallStaticBooleanMethod(cls, mid, key, static_cast<jboolean>(false));
    });
}

bool AndroidControlUnitMgr::scroll(int dx, int dy)
{
    return call_bool("scroll", "(II)Z", [&](JNIEnv* env, jclass cls, jmethodID mid) {
        return env->CallStaticBooleanMethod(cls, mid, dx, dy);
    });
}

MAA_CTRL_UNIT_NS_END

// JNI_OnLoad to capture JavaVM
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
    MAA_CTRL_UNIT_NS::g_vm = vm;
    return JNI_VERSION_1_6;
}
