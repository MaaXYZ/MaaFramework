#pragma once

#include "Base/UnitBase.h"
#include "Common/Conf.h"

#include <ApplicationServices/ApplicationServices.h>

MAA_CTRL_UNIT_NS_BEGIN

// 通过 CGEventPost(kCGHIDEventTap) 向全局 HID 事件流注入鼠标/键盘事件。
// 事件不针对特定进程，由系统自动分发至当前前台窗口。
// 会自动激活目标窗口。
class GlobalEventInput : public InputBase
{
public:
    GlobalEventInput(uint32_t window_id)
        : window_id_(window_id)
    {
        update_window_info();
    }

    virtual ~GlobalEventInput() override = default;

public: // from InputBase
    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

private:
    bool activate_window(pid_t target_pid);
    void update_window_info();

    // 工具函数：创建、发送并释放 CGEvent
    bool post_mouse_event(CGEventType type, CGPoint location, CGMouseButton button = kCGMouseButtonLeft);
    bool post_keyboard_event(CGKeyCode key_code, bool key_down);

    uint32_t window_id_ = 0;
    pid_t pid_ = -1;
    int offset_x_ = 0;
    int offset_y_ = 0;
    CGPoint saved_cursor_pos_ = { NAN, NAN };
};

MAA_CTRL_UNIT_NS_END
