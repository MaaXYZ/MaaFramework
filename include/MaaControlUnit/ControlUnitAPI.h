#pragma once

#include <chrono>
#include <string>
#include <utility>

#include <meojson/json.hpp>

#include <opencv2/core/mat.hpp>

#include "MaaControlUnit/MaaControlUnitDef.h"
#include "MaaFramework/MaaDef.h"

MAA_CTRL_UNIT_NS_BEGIN

class ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI() = default;

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

class ScrollableUnit
{
public:
    virtual ~ScrollableUnit() = default;

    virtual bool scroll(int dx, int dy) = 0;
};

class RelativeMovableUnit
{
public:
    virtual ~RelativeMovableUnit() = default;

    virtual bool relative_move(int dx, int dy) = 0;
};

class ShellableUnit
{
public:
    virtual ~ShellableUnit() = default;

    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) = 0;
};

// Platform-specific APIs, composed from base + capability mixins

class AdbControlUnitAPI
    : public ControlUnitAPI
    , public ShellableUnit
{
public:
    virtual ~AdbControlUnitAPI() = default;

    virtual bool find_device(/*out*/ std::vector<std::string>& devices) = 0;
};

class AndroidNativeControlUnitAPI : public ControlUnitAPI
{
public:
    ~AndroidNativeControlUnitAPI() override = default;
};

class Win32ControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    virtual ~Win32ControlUnitAPI() = default;

    virtual bool set_mouse_lock_follow(bool /*enabled*/) { return false; }
};

class MacOSControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
{
public:
    virtual ~MacOSControlUnitAPI() = default;
};

class CustomControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
    , public ShellableUnit
{
public:
    virtual ~CustomControlUnitAPI() = default;
};

class GamepadControlUnitAPI : public ControlUnitAPI
{
public:
    virtual ~GamepadControlUnitAPI() = default;
};

class FullControlUnitAPI
    : public ControlUnitAPI
    , public ScrollableUnit
    , public RelativeMovableUnit
    , public ShellableUnit
{
public:
    virtual ~FullControlUnitAPI() = default;
};

MAA_CTRL_UNIT_NS_END

using MaaControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
using MaaAdbControlUnitHandle = MAA_CTRL_UNIT_NS::AdbControlUnitAPI*;
using MaaWin32ControlUnitHandle = MAA_CTRL_UNIT_NS::Win32ControlUnitAPI*;
using MaaMacOSControlUnitHandle = MAA_CTRL_UNIT_NS::MacOSControlUnitAPI*;
using MaaGamepadControlUnitHandle = MAA_CTRL_UNIT_NS::GamepadControlUnitAPI*;
using MaaCustomControlUnitHandle = MAA_CTRL_UNIT_NS::CustomControlUnitAPI*;
using MaaReplayControlUnitHandle = MAA_CTRL_UNIT_NS::FullControlUnitAPI*;
using MaaRecordControlUnitHandle = MAA_CTRL_UNIT_NS::FullControlUnitAPI*;
using MaaDbgControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
using MaaAndroidNativeControlUnitHandle = MAA_CTRL_UNIT_NS::AndroidNativeControlUnitAPI*;
