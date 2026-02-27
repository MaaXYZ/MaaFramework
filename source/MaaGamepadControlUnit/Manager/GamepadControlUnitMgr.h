#pragma once

#include <memory>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ViGEmInput;
class Win32ControlUnitLoader;

class GamepadControlUnitMgr : public Win32ControlUnitAPI
{
public:
    GamepadControlUnitMgr(HWND hWnd, MaaGamepadType gamepad_type, MaaWin32ScreencapMethod screencap_method);
    virtual ~GamepadControlUnitMgr() override;

public: // from ControlUnitAPI
    virtual bool connect() override;
    virtual bool connected() const override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;
    virtual MaaControllerFeature get_features() const override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    // touch functions repurposed for analog inputs:
    // contact 0: left stick (x: -32768~32767, y: -32768~32767)
    // contact 1: right stick (x: -32768~32767, y: -32768~32767)
    // contact 2: left trigger (pressure: 0~255, x/y ignored)
    // contact 3: right trigger (pressure: 0~255, x/y ignored)
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    // key functions for gamepad buttons
    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

    virtual bool inactive() override;

private:
    bool init_win32_unit();
    void ensure_foreground();

private:
    HWND hwnd_ = nullptr;
    MaaGamepadType gamepad_type_ = MaaGamepadType_Xbox360;
    MaaWin32ScreencapMethod screencap_method_ = MaaWin32ScreencapMethod_None;

    bool connected_ = false;

    std::unique_ptr<ViGEmInput> gamepad_input_;
    std::unique_ptr<Win32ControlUnitLoader> win32_loader_; // DLL loader
    std::shared_ptr<ControlUnitAPI> win32_unit_;           // for screencap
};

MAA_CTRL_UNIT_NS_END
