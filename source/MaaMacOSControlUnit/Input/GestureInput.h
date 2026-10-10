#pragma once

#include "PostToPidInput.h"

MAA_CTRL_UNIT_NS_BEGIN

// Background single-contact gestures for native iOS apps; keyboard input is inherited.
class GestureInput : public PostToPidInput
{
public:
    GestureInput(uint32_t window_id)
        : PostToPidInput(window_id)
    {
    }

    virtual ~GestureInput() override;

public: // from InputBase
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

private:
    struct GestureRoute
    {
        using PostToPid = void (*)(pid_t, CGEventRef);
        using SetWindowLocation = void (*)(CGEventRef, CGPoint);

        void* framework = nullptr;
        PostToPid post = nullptr;
        SetWindowLocation set_window_location = nullptr;

        static GestureRoute& get_instance();
    };

    bool post_gesture(CGScrollPhase phase, int x, int y, double dx, double dy);
    bool touching_ = false;
    bool seed_pending_ = false;

    uint64_t event_timestamp_ns() const;
    bool post_event(CGEventRef event, CGPoint local, CGPoint screen) const;
};

MAA_CTRL_UNIT_NS_END
