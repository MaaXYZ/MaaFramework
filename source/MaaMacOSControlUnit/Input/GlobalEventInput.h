#pragma once

#include "Base/UnitBase.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class GlobalEventInput : public InputBase
{
public:
    GlobalEventInput(uint32_t window_id, pid_t pid = 0)
        : window_id_(window_id)
        , pid_(pid == 0 ? get_window_pid(window_id) : pid)
    {
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
    pid_t get_window_pid(uint32_t window_id);

    uint32_t window_id_ = 0;
    pid_t pid_ = -1;
};

MAA_CTRL_UNIT_NS_END
