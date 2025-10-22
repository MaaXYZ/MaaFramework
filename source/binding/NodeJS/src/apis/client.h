#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>

#include <MaaAgentClient/MaaAgentClientAPI.h>
#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

struct ClientImpl : public maajs::NativeClassBase
{
    MaaAgentClient* client {};

    ClientImpl() = default;
    ClientImpl(std::string identifier);
    ~ClientImpl();
    void destroy();

    std::string to_string() override;

    constexpr static char name[] = "Client";

    static ClientImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

