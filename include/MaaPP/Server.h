#pragma once

#include <map>

#include <MaaAgentServer/MaaAgentServerAPI.h>

#include "MaaPP/Custom.h"
#include "MaaPP/Exception.h"

namespace maapp::Server
{

inline std::map<std::string, CustomRecognition> recos;
inline std::map<std::string, CustomAction> actions;

inline void register_custom_recognition(const std::string& name, CustomRecognition reco)
{
    if (!MaaAgentServerRegisterCustomRecognition(
            name.c_str(),
            &pri::custom_reco_wrapper<
                int,
                +[](void*, const char* name) {
                    return std::make_tuple(recos[name], 0);
                }>,
            nullptr)) {
        throw FunctionFailed("MaaAgentServerRegisterCustomRecognition");
    }
    recos[name] = reco;
}

inline void register_custom_action(const std::string& name, CustomAction act)
{
    if (!MaaAgentServerRegisterCustomAction(
            name.c_str(),
            &pri::custom_act_wrapper<
                int,
                +[](void*, const char* name) {
                    return std::make_tuple(actions[name], 0);
                }>,
            nullptr)) {
        throw FunctionFailed("MaaAgentServerRegisterCustomAction");
    }
    actions[name] = act;
}

inline void start_up(const std::string& identifier)
{
    if (!MaaAgentServerStartUp(identifier.c_str())) {
        throw FunctionFailed("MaaAgentServerStartUp");
    }
}

inline void shut_down()
{
    MaaAgentServerShutDown();
}

inline void join()
{
    MaaAgentServerJoin();
}

inline void detach()
{
    MaaAgentServerDetach();
}

}
