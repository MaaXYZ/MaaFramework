#pragma once

#include "Common/MaaTypes.h"
#include "MaaAgent/Transceiver.h"

MAA_AGENT_SERVER_NS_BEGIN

class RemoteController : public MaaController
{
public:
    RemoteController(Transceiver& server, const std::string& controller_id);
    virtual ~RemoteController() = default;

    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaCtrlId post_connection() override;
    virtual MaaCtrlId post_click(int x, int y) override;
    virtual MaaCtrlId post_swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual MaaCtrlId post_press_key(int keycode) override;
    virtual MaaCtrlId post_input_text(const std::string& text) override;
    virtual MaaCtrlId post_start_app(const std::string& intent) override;
    virtual MaaCtrlId post_stop_app(const std::string& intent) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_up(int contact) override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const override;
    virtual bool connected() const override;
    virtual bool running() const override;

    virtual cv::Mat cached_image() const override;
    virtual std::string get_uuid() override;

private:
    Transceiver& server_;
    std::string controller_id_;
};

MAA_AGENT_SERVER_NS_END
