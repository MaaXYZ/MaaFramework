#pragma once

#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "Base/HeldInput.h"
#include "Base/UnitBase.h"
#include "Input/BackgroundManagedKeyInput.h"
#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class Win32ControlUnitMgr
    : public Win32ControlUnitAPI
    , public BindableUnit
{
public:
    Win32ControlUnitMgr(
        HWND hWnd,
        MaaWin32ScreencapMethod screencap_method,
        MaaWin32InputMethod mouse_method,
        MaaWin32InputMethod keyboard_method);
    ~Win32ControlUnitMgr() override;

public: // from ControlUnitAPI
    virtual bool connect() override;
    virtual bool connected() const override;

    bool set_deferred_binding() override;
    bool bind_target(const json::object& target, const std::function<bool()>& cancelled, const std::function<void()>& before_switch)
        override;
    bool target_available() const override;

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

    virtual bool relative_move(int dx, int dy) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

    virtual bool set_mouse_lock_follow(bool enabled) override;
    virtual bool set_background_managed_keys_option(const int32_t* keycodes, size_t count) override;

    virtual bool inactive() override;

    virtual json::object get_info() const override;

private:
    enum class ScreencapMethod
    {
        UnknownYet,
        GDI,
        FramePool,
        DXGI_DesktopDup,
        DXGI_DesktopDup_Window,
        PrintWindow,
        ScreenDC,
    };

    struct Target
    {
        HWND hwnd = nullptr;
        DWORD process_id = 0;
        DWORD thread_id = 0;
        bool ready = false;
        mutable bool lost = false;
        std::shared_ptr<InputBase> mouse;
        std::shared_ptr<InputBase> keyboard;
        std::shared_ptr<ScreencapBase> screencap;
        std::shared_ptr<BackgroundManagedKeyInput> background_keyboard;

        HeldInput contacts;
        HeldInput keys;
        HeldInput guarded_keys;

        bool available() const;
        bool release_key(int key);
        bool release_contact(int contact);
        bool release_guarded_key(int key);
        bool release_held();
        void inactive();
    };

    bool prepare_target(Target& target, bool explicit_bind, const std::function<bool()>& cancelled = { }) const;
    std::unordered_map<ScreencapMethod, std::shared_ptr<ScreencapBase>>
        build_screencap_units(HWND hwnd, MaaWin32ScreencapMethod methods) const;
    bool init_screencap(Target& target, MaaWin32ScreencapMethod methods, const std::function<bool()>& cancelled) const;
    std::shared_ptr<InputBase> make_input(HWND hwnd, MaaWin32InputMethod method) const;

    std::shared_ptr<ScreencapBase> speed_test(
        const std::unordered_map<ScreencapMethod, std::shared_ptr<ScreencapBase>>& units,
        const std::function<bool()>& cancelled) const;

    const HWND initial_hwnd_;
    const MaaWin32ScreencapMethod screencap_method_;
    const MaaWin32InputMethod mouse_method_;
    const MaaWin32InputMethod keyboard_method_;

    // Queries and option setters can also be called outside the controller queue.
    mutable std::mutex mutex_;
    std::mutex connection_mutex_;
    Target target_;
    bool connected_ = false;
    bool independent_connection_ = false;
    std::string session_uuid_;
    std::unordered_set<int> managed_keys_;
};

MAA_CTRL_UNIT_NS_END
