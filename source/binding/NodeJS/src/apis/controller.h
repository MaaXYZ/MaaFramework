#pragma once

#include <tuple>

#include "../foundation/convert.h"
#include "job.h"

struct ImageJobImpl : public JobImpl
{
    virtual maajs::ValueType get(maajs::EnvType env) override;

    constexpr static char name[] = "ImageJob";

    static ImageJobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::EnvType env, maajs::ObjectType proto, maajs::FunctionType ctor);
};

struct ControllerImpl : public maajs::NativeClassBase
{
    MaaController* controller {};
    bool own = false;

    ControllerImpl() = default;
    ControllerImpl(MaaController* ctrl, bool own);
    ~ControllerImpl();
    void destroy();
    maajs::ValueType post_connection(maajs::ValueType self, maajs::EnvType env);
    maajs::ValueType post_screencap(maajs::ValueType self, maajs::EnvType env);
    MaaStatus status(MaaCtrlId id);
    maajs::PromiseType wait(maajs::EnvType env, MaaCtrlId id);
    bool get_connected();
    std::optional<maajs::ArrayBufferType> get_cached_image(maajs::EnvType env);
    std::optional<std::string> get_uuid();

    constexpr static char name[] = "Controller";

    static ControllerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::EnvType env, maajs::ObjectType proto, maajs::FunctionType ctor);
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
    static void init_proto(maajs::EnvType env, maajs::ObjectType proto, maajs::FunctionType ctor);
};
