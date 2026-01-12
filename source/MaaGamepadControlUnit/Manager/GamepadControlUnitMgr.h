#pragma once

#include <memory>
#include <string>

#include "ControlUnit/ControlUnitAPI.h"
#include "ControlUnit/Win32ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/SafeWindows.hpp"

#include <ViGEm/Client.h>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class Win32ScreencapLibraryHolder : public MAA_NS::LibraryHolder<Win32ScreencapLibraryHolder>
{
public:
    static std::shared_ptr<Win32ControlUnitAPI> create_control_unit(void* hWnd, MaaWin32ScreencapMethod screencap_method);
};

class GamepadControlUnitMgr : public ControlUnitAPI
{
public:
    GamepadControlUnitMgr(HWND hWnd, MaaWin32ScreencapMethod screencap_method);
    virtual ~GamepadControlUnitMgr() override;

public: // from ControlUnitAPI
    virtual bool connect() override;
    virtual bool connected() const override;

    virtual bool request_uuid(std::string& uuid) override;
    virtual MaaControllerFeature get_features() const override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(cv::Mat& image) override;

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
    bool load_vigem();
    void unload_vigem();
    bool init_gamepad();
    void uninit_gamepad();
    bool send_state();

    bool set_button(WORD button, bool pressed);
    bool set_left_stick(SHORT x, SHORT y);
    bool set_right_stick(SHORT x, SHORT y);
    bool set_left_trigger(BYTE value);
    bool set_right_trigger(BYTE value);

    HWND hwnd_ = nullptr;
    MaaWin32ScreencapMethod screencap_method_ = MaaWin32ScreencapMethod_None;
    std::shared_ptr<Win32ControlUnitAPI> win32_unit_ = nullptr;

    HMODULE vigem_module_ = nullptr;
    vigem::PVIGEM_CLIENT client_ = nullptr;
    vigem::PVIGEM_TARGET pad_ = nullptr;
    vigem::XUSB_REPORT report_ = {};
    bool gamepad_inited_ = false;

    // Function pointers
    vigem::PFN_vigem_alloc fn_alloc_ = nullptr;
    vigem::PFN_vigem_free fn_free_ = nullptr;
    vigem::PFN_vigem_connect fn_connect_ = nullptr;
    vigem::PFN_vigem_disconnect fn_disconnect_ = nullptr;
    vigem::PFN_vigem_target_x360_alloc fn_target_x360_alloc_ = nullptr;
    vigem::PFN_vigem_target_free fn_target_free_ = nullptr;
    vigem::PFN_vigem_target_add fn_target_add_ = nullptr;
    vigem::PFN_vigem_target_remove fn_target_remove_ = nullptr;
    vigem::PFN_vigem_target_x360_update fn_target_x360_update_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
