#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "job.h"

struct ImageJobImpl : public JobImpl
{
    virtual maajs::ValueType get() override;

    constexpr static char name[] = "ImageJob";

    static ImageJobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

struct ControllerImpl : public maajs::NativeClassBase
{
    MaaController* controller {};
    bool own = false;
    std::map<MaaSinkId, maajs::CallbackContext*> sinks {};

    ControllerImpl() = default;
    ControllerImpl(MaaController* ctrl, bool own);
    ~ControllerImpl();
    void destroy();
    MaaSinkId add_sink(maajs::FunctionType sink);
    void remove_sink(MaaSinkId id);
    void clear_sinks();
    void set_screenshot_target_long_side(int32_t value);
    void set_screenshot_target_short_side(int32_t value);
    void set_screenshot_use_raw_size(bool value);
    maajs::ValueType post_connection(maajs::ValueType self, maajs::EnvType env);
    maajs::ValueType post_screencap(maajs::ValueType self, maajs::EnvType env);
    MaaStatus status(MaaCtrlId id);
    maajs::PromiseType wait(MaaCtrlId id);
    bool get_connected();
    std::optional<maajs::ArrayBufferType> get_cached_image();
    std::optional<std::string> get_uuid();

    std::string to_string() override;

    static maajs::ValueType locate_object(maajs::EnvType env, MaaController* ctrl);

    constexpr static char name[] = "Controller";

    virtual void init_bind(maajs::ObjectType self) override;
    virtual void gc_mark(maajs::NativeMarkerFunc marker) override;
    static ControllerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

using AdbDevice = std::tuple<std::string, std::string, std::string, MaaAdbScreencapMethod, MaaAdbInputMethod, std::string>;
using AdbControllerCtorParam =
    std::tuple<std::string, std::string, MaaAdbScreencapMethod, MaaAdbInputMethod, std::string, maajs::OptionalParam<std::string>>;

struct AdbControllerImpl : public ControllerImpl
{
    using ControllerImpl::ControllerImpl;

    static std::string agent_path();
    static maajs::PromiseType find(maajs::EnvType env, maajs::OptionalParam<std::string> adb);

    constexpr static char name[] = "AdbController";

    static AdbControllerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

using Win32Device = std::tuple<uintptr_t, std::string, std::string>;
using Win32ControllerCtorParam = std::tuple<uintptr_t, MaaWin32ScreencapMethod, MaaWin32InputMethod>;

struct Win32ControllerImpl : public ControllerImpl
{
    using ControllerImpl::ControllerImpl;

    static maajs::PromiseType find(maajs::EnvType env);

    constexpr static char name[] = "Win32Controller";

    static Win32ControllerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

using DbgControllerCtorParam = std::tuple<std::string, std::string, MaaDbgControllerType, std::string>;

struct DbgControllerImpl : public ControllerImpl
{
    using ControllerImpl::ControllerImpl;

    constexpr static char name[] = "DbgController";

    static DbgControllerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

struct CustomControllerContext
{
    std::map<std::string, maajs::CallbackContext*> callbacks;

    ~CustomControllerContext();
    void add_bind(maajs::EnvType env, std::string name, std::string func_name, int argc, std::shared_ptr<maajs::ObjectRefType> actor);
    void gc_mark(maajs::NativeMarkerFunc marker);
};

struct CustomControllerImpl : public ControllerImpl
{
    std::shared_ptr<maajs::ObjectRefType> actor;
    std::unique_ptr<CustomControllerContext> context;

    using ControllerImpl::ControllerImpl;

    constexpr static char name[] = "CustomController";

    virtual void gc_mark(maajs::NativeMarkerFunc marker) override;
    static CustomControllerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};
