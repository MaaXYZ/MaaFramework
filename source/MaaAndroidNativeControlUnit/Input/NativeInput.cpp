#include "NativeInput.h"

#include <chrono>
#include <thread>

#include "MaaUtils/Logger.h"

// Android MotionEvent action constants
#define AMOTION_EVENT_ACTION_DOWN 0
#define AMOTION_EVENT_ACTION_UP 1
#define AMOTION_EVENT_ACTION_MOVE 2
#define AMOTION_EVENT_ACTION_POINTER_DOWN 5
#define AMOTION_EVENT_ACTION_POINTER_UP 6

// Android KeyEvent action constants
#define AKEY_EVENT_ACTION_DOWN 0
#define AKEY_EVENT_ACTION_UP 1

MAA_CTRL_UNIT_NS_BEGIN

NativeInput::~NativeInput()
{
    LogFunc;

    if (input_helper_ && jvm_) {
        JNIEnv* env = nullptr;
        if (jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK && env) {
            env->DeleteGlobalRef(input_helper_);
        }
    }
}

bool NativeInput::init(int display_width, int display_height)
{
    LogFunc << VAR(display_width) << VAR(display_height);

    display_width_ = display_width;
    display_height_ = display_height;

    // JNI 初始化将在外部完成
    // 这里只记录显示尺寸

    return true;
}

bool NativeInput::inject_touch_event(int action, int x, int y, int pointer_id)
{
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

    if (!input_helper_ || !inject_touch_method_) {
        LogError << "input helper not initialized";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    jboolean result = env->CallBooleanMethod(input_helper_, inject_touch_method_, action, x, y, pointer_id);

    if (need_detach) {
        jvm_->DetachCurrentThread();
    }

    return result == JNI_TRUE;
}

bool NativeInput::inject_key_event(int action, int keycode)
{
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

    if (!input_helper_ || !inject_key_method_) {
        LogError << "input helper not initialized";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    jboolean result = env->CallBooleanMethod(input_helper_, inject_key_method_, action, keycode);

    if (need_detach) {
        jvm_->DetachCurrentThread();
    }

    return result == JNI_TRUE;
}

bool NativeInput::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    if (!touch_down(0, x, y, 50)) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return touch_up(0);
}

bool NativeInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!touch_down(0, x1, y1, 50)) {
        return false;
    }

    const int steps = duration / 10;
    const double dx = static_cast<double>(x2 - x1) / steps;
    const double dy = static_cast<double>(y2 - y1) / steps;

    for (int i = 1; i <= steps; ++i) {
        int x = static_cast<int>(x1 + dx * i);
        int y = static_cast<int>(y1 + dy * i);

        if (!touch_move(0, x, y, 50)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return touch_up(0);
}

bool NativeInput::touch_down(int contact, int x, int y, [[maybe_unused]] int pressure)
{
    int action = (contact == 0) ? AMOTION_EVENT_ACTION_DOWN : (AMOTION_EVENT_ACTION_POINTER_DOWN | (contact << 8));
    return inject_touch_event(action, x, y, contact);
}

bool NativeInput::touch_move(int contact, int x, int y, [[maybe_unused]] int pressure)
{
    return inject_touch_event(AMOTION_EVENT_ACTION_MOVE, x, y, contact);
}

bool NativeInput::touch_up(int contact)
{
    int action = (contact == 0) ? AMOTION_EVENT_ACTION_UP : (AMOTION_EVENT_ACTION_POINTER_UP | (contact << 8));
    return inject_touch_event(action, 0, 0, contact);
}

bool NativeInput::click_key(int key)
{
    LogInfo << VAR(key);

    if (!key_down(key)) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return key_up(key);
}

bool NativeInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

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

    if (!input_helper_ || !inject_text_method_) {
        LogError << "input helper not initialized";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    jstring jtext = env->NewStringUTF(text.c_str());
    jboolean result = env->CallBooleanMethod(input_helper_, inject_text_method_, jtext);
    env->DeleteLocalRef(jtext);

    if (need_detach) {
        jvm_->DetachCurrentThread();
    }

    return result == JNI_TRUE;
}

bool NativeInput::key_down(int key)
{
    return inject_key_event(AKEY_EVENT_ACTION_DOWN, key);
}

bool NativeInput::key_up(int key)
{
    return inject_key_event(AKEY_EVENT_ACTION_UP, key);
}

bool NativeInput::start_app(const std::string& intent)
{
    LogFunc << VAR(intent);

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

    if (!input_helper_ || !start_app_method_) {
        LogError << "input helper not initialized";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    jstring jintent = env->NewStringUTF(intent.c_str());
    jboolean result = env->CallBooleanMethod(input_helper_, start_app_method_, jintent);
    env->DeleteLocalRef(jintent);

    if (need_detach) {
        jvm_->DetachCurrentThread();
    }

    return result == JNI_TRUE;
}

bool NativeInput::stop_app(const std::string& intent)
{
    LogFunc << VAR(intent);

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

    if (!input_helper_ || !stop_app_method_) {
        LogError << "input helper not initialized";
        if (need_detach) {
            jvm_->DetachCurrentThread();
        }
        return false;
    }

    jstring jintent = env->NewStringUTF(intent.c_str());
    jboolean result = env->CallBooleanMethod(input_helper_, stop_app_method_, jintent);
    env->DeleteLocalRef(jintent);

    if (need_detach) {
        jvm_->DetachCurrentThread();
    }

    return result == JNI_TRUE;
}

MAA_CTRL_UNIT_NS_END
