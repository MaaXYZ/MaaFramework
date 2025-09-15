#pragma once

#include <filesystem>

#include "Base/UnitBase.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

MAA_CTRL_UNIT_NS_BEGIN

class MacControlUnitMgr : public MacControlUnitAPI
{
public:
    MacControlUnitMgr(uint32_t windowId, MaaMacScreencapMethod screencap_method, MaaMacInputMethod input_method);
    virtual ~MacControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual bool connect() override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool is_touch_availabled() const override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool is_key_down_up_availabled() const override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

private:
    uint32_t window_id_ = 0;
    MaaMacScreencapMethod screencap_method_ = MaaMacScreencapMethod_None;
    MaaMacInputMethod input_method_ = MaaMacInputMethod_None;

    std::shared_ptr<InputBase> input_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
