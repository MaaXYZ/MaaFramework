#pragma once

#include <jni.h>

#include "Base/InputBase.h"

#include "Conf/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// 使用 Instrumentation 或 InputManager 注入输入事件
class NativeInput : public InputBase
{
public:
    NativeInput() = default;
    virtual ~NativeInput() override;

public:
    virtual bool init(int display_width, int display_height) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

private:
    bool inject_touch_event(int action, int x, int y, int pointer_id = 0);
    bool inject_key_event(int action, int keycode);

    int display_width_ = 0;
    int display_height_ = 0;

    // JNI 相关
    JavaVM* jvm_ = nullptr;
    jobject input_helper_ = nullptr;
    jmethodID inject_touch_method_ = nullptr;
    jmethodID inject_key_method_ = nullptr;
    jmethodID inject_text_method_ = nullptr;
    jmethodID start_app_method_ = nullptr;
    jmethodID stop_app_method_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
