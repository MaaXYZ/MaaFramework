#pragma once

#include <map>
#include <tuple>

#include "../foundation/callback.h"
#include "../foundation/convert.h"
#include "job.h"

struct ImageJobImpl : public JobImpl
{
    virtual maajs::ValueType get() override;

    constexpr static char name[] = "ImageJob";

    static ImageJobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

MAA_JS_NATIVE_CLASS_STATIC_FORWARD(ImageJobImpl)

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

MAA_JS_NATIVE_CLASS_STATIC_FORWARD(ControllerImpl)

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

MAA_JS_NATIVE_CLASS_STATIC_FORWARD(AdbControllerImpl)
