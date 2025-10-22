#include "loader.h"

#include <MaaAgentServer/MaaAgentServerAPI.h>

#include "../foundation/spec.h"
#include "callback.h"

static void register_custom_recognition(std::string key, maajs::FunctionType func)
{
    auto ctx = new maajs::CallbackContext(func, "CustomReco");
    if (!MaaAgentServerRegisterCustomRecognition(key.c_str(), CustomReco, ctx)) {
        throw maajs::MaaError { "Server register_custom_recognition failed" };
    }
}

static void register_custom_action(std::string key, maajs::FunctionType func)
{
    auto ctx = new maajs::CallbackContext(func, "CustomAct");
    if (!MaaAgentServerRegisterCustomAction(key.c_str(), CustomAct, ctx)) {
        throw maajs::MaaError { "Server register_custom_action failed" };
    }
}

static maajs::PromiseType start_up(maajs::EnvType env, std::string identifier)
{
    auto work = new maajs::AsyncWork<bool>(env, [identifier]() { return MaaAgentServerStartUp(identifier.c_str()); });
    work->Queue();
    return work->Promise();
}

static maajs::PromiseType shut_down(maajs::EnvType env)
{
    auto work = new maajs::AsyncWork<std::monostate>(env, []() {
        MaaAgentServerShutDown();
        return std::monostate {};
    });
    work->Queue();
    return work->Promise();
}

static maajs::PromiseType join(maajs::EnvType env)
{
    auto work = new maajs::AsyncWork<std::monostate>(env, []() {
        MaaAgentServerJoin();
        return std::monostate {};
    });
    work->Queue();
    return work->Promise();
}

static void detach()
{
    MaaAgentServerDetach();
}

maajs::ValueType load_server(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    MAA_BIND_FUNC(obj, "register_custom_recognition", register_custom_recognition);
    MAA_BIND_FUNC(obj, "register_custom_action", register_custom_action);
    MAA_BIND_FUNC(obj, "start_up", start_up);
    MAA_BIND_FUNC(obj, "shut_down", shut_down);
    MAA_BIND_FUNC(obj, "join", join);
    MAA_BIND_FUNC(obj, "detach", detach);

    return obj;
}
