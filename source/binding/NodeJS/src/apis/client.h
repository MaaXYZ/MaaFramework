#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <MaaAgentClient/MaaAgentClientAPI.h>
#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "controller.h"
#include "resource.h"
#include "tasker.h"

struct ClientImpl : public maajs::NativeClassBase
{
    MaaAgentClient* client {};

    ClientImpl() = default;
    ClientImpl(std::string identifier);
    ~ClientImpl();
    void destroy();
    std::string get_identifier();
    void bind_resource(maajs::NativeObject<ResourceImpl> resource);
    void register_resource_sink(maajs::NativeObject<ResourceImpl> resource);
    void register_controller_sink(maajs::NativeObject<ControllerImpl> controller);
    void register_tasker_sink(maajs::NativeObject<TaskerImpl>);
    maajs::PromiseType connect();
    void disconnect();
    bool get_connected();
    bool get_alive();
    void set_timeout(uint64_t ms);
    std::optional<std::vector<std::string>> get_custom_recognition_list();
    std::optional<std::vector<std::string>> get_custom_action_list();

    std::string to_string() override;

    constexpr static char name[] = "Client";

    static ClientImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

