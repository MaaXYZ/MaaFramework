#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include <meojson/json.hpp>

#include <opencv2/core/mat.hpp>

#include "MaaControlUnit/MaaControlUnitDef.h"
#include "MaaFramework/MaaDef.h"

extern "C" MAA_CONTROL_UNIT_TYPES_API const char* MaaControlUnitGetVersion();

MAA_CTRL_UNIT_NS_BEGIN

class MAA_CONTROL_UNIT_TYPES_API ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI();

    virtual bool connect() = 0;
    virtual bool connected() const = 0;

    virtual bool request_uuid(/*out*/ std::string& uuid) = 0;
    virtual MaaControllerFeature get_features() const = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;

    virtual bool screencap(/*out*/ cv::Mat& image) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool inactive() = 0;

    virtual json::object get_info() const = 0;
};

// Capability mixins: optional interfaces that controllers may support

class MAA_CONTROL_UNIT_TYPES_API ScrollableUnit
{
public:
    virtual ~ScrollableUnit();

    virtual bool scroll(int dx, int dy) = 0;
};

class MAA_CONTROL_UNIT_TYPES_API RelativeMovableUnit
{
public:
    virtual ~RelativeMovableUnit();

    virtual bool relative_move(int dx, int dy) = 0;
};

class MAA_CONTROL_UNIT_TYPES_API ShellableUnit
{
public:
    virtual ~ShellableUnit();

    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) = 0;
};

// Platform-specific APIs, composed from base + capability mixins

class MAA_CONTROL_UNIT_TYPES_API AdbControlUnitAPI
    : public ControlUnitAPI
    , public ShellableUnit
{
public:
    ~AdbControlUnitAPI() override;

    virtual bool find_device(/*out*/ std::vector<std::string>& devices) = 0;
};

class MAA_CONTROL_UNIT_TYPES_API IOSControlUnitAPI : public ControlUnitAPI
{
public:
    ~IOSControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API AndroidNativeControlUnitAPI : public ControlUnitAPI
{
public:
    ~AndroidNativeControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API Win32ControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    ~Win32ControlUnitAPI() override;

    virtual bool set_mouse_lock_follow(bool /*enabled*/) { return false; }

    virtual bool set_background_managed_keys_option(const int32_t* /*keycodes*/, size_t /*count*/) { return false; }
};

class MAA_CONTROL_UNIT_TYPES_API MacOSControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    ~MacOSControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API WlRootsControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    ~WlRootsControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API KWinControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    ~KWinControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API LinuxControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    ~LinuxControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API CustomControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
    , public ShellableUnit
{
public:
    ~CustomControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API GamepadControlUnitAPI : public ControlUnitAPI
{
public:
    ~GamepadControlUnitAPI() override;
};

class MAA_CONTROL_UNIT_TYPES_API FullControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
    , public ShellableUnit
{
public:
    ~FullControlUnitAPI() override;
};

MAA_CTRL_UNIT_NS_END

using MaaControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
using MaaAdbControlUnitHandle = MAA_CTRL_UNIT_NS::AdbControlUnitAPI*;
using MaaIOSControlUnitHandle = MAA_CTRL_UNIT_NS::IOSControlUnitAPI*;
using MaaWin32ControlUnitHandle = MAA_CTRL_UNIT_NS::Win32ControlUnitAPI*;
using MaaMacOSControlUnitHandle = MAA_CTRL_UNIT_NS::MacOSControlUnitAPI*;
using MaaLinuxControlUnitHandle = MAA_CTRL_UNIT_NS::LinuxControlUnitAPI*;
using MaaWlRootsControlUnitHandle = MAA_CTRL_UNIT_NS::LinuxControlUnitAPI*;
using MaaKWinControlUnitHandle = MAA_CTRL_UNIT_NS::LinuxControlUnitAPI*;
using MaaGamepadControlUnitHandle = MAA_CTRL_UNIT_NS::GamepadControlUnitAPI*;
using MaaCustomControlUnitHandle = MAA_CTRL_UNIT_NS::CustomControlUnitAPI*;
using MaaReplayControlUnitHandle = MAA_CTRL_UNIT_NS::FullControlUnitAPI*;
using MaaRecordControlUnitHandle = MAA_CTRL_UNIT_NS::FullControlUnitAPI*;
using MaaDbgControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
using MaaAndroidNativeControlUnitHandle = MAA_CTRL_UNIT_NS::AndroidNativeControlUnitAPI*;
