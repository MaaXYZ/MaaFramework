#pragma once

#include <filesystem>

#include "Base/UnitBase.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class Win32ControlUnitMgr : public Win32ControlUnitAPI
{
public:
    Win32ControlUnitMgr(
        HWND hWnd,
        MaaWin32ScreencapMethod screencap_method,
        MaaWin32InputMethod mouse_method,
        MaaWin32InputMethod keyboard_method);
    virtual ~Win32ControlUnitMgr() override = default;

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

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

    virtual bool inactive() override;

private:
    HWND hwnd_ = nullptr;
    MaaWin32ScreencapMethod screencap_method_ = MaaWin32ScreencapMethod_None;
    MaaWin32InputMethod mouse_method_ = MaaWin32InputMethod_None;
    MaaWin32InputMethod keyboard_method_ = MaaWin32InputMethod_None;

    bool connected_ = false;
    std::shared_ptr<InputBase> mouse_ = nullptr;
    std::shared_ptr<InputBase> keyboard_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
