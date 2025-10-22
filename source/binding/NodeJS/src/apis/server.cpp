#include "loader.h"

#include <MaaAgentServer/MaaAgentServerAPI.h>

#include "../foundation/spec.h"

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

    MAA_BIND_FUNC(obj, "start_up", start_up);
    MAA_BIND_FUNC(obj, "shut_down", shut_down);
    MAA_BIND_FUNC(obj, "join", join);
    MAA_BIND_FUNC(obj, "detach", detach);

    return obj;
}
