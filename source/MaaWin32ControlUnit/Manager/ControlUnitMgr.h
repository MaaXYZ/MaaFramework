#pragma once

#include <filesystem>

#include "Base/UnitBase.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ControlUnitMgr : public ControlUnitAPI
{
public:
    ControlUnitMgr(HWND hWnd, MaaWin32ScreencapMethod screencap_method, MaaWin32InputMethod input_method);
    virtual ~ControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual bool find_device(/*out*/ std::vector<std::string>& devices) override;

    virtual bool connect() override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual bool multi_swipe(const std::vector<SwipeParam>& swipes) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    HWND hwnd_ = nullptr;
    MaaWin32ScreencapMethod screencap_method_ = MaaWin32ScreencapMethod_None;
    MaaWin32InputMethod input_method_ = MaaWin32InputMethod_None;

    std::shared_ptr<InputBase> input_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
