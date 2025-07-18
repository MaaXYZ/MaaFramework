#pragma once

#include <cstdint>
#include <map>

#include <MaaToolkit/MaaToolkitAPI.h>

#include "MaaPP/Custom.h"
#include "MaaPP/Exception.h"

namespace maapp::ProjectInterface
{

inline std::map<uint64_t, std::map<std::string, CustomRecognition>> recos;
inline std::map<uint64_t, std::map<std::string, CustomAction>> actions;

inline void register_custom_recognition(uint64_t inst, const std::string& name, CustomRecognition reco)
{
    MaaToolkitProjectInterfaceRegisterCustomRecognition(
        inst,
        name.c_str(),
        &pri::custom_reco_wrapper<
            int,
            +[](void* arg, const char* name) {
                auto inst = reinterpret_cast<uintptr_t>(arg);
                return std::make_tuple(recos[inst][name], 0);
            }>,
        reinterpret_cast<void*>(inst));
    recos[inst][name] = reco;
}

inline void register_custom_action(uint64_t inst, const std::string& name, CustomAction act)
{
    MaaToolkitProjectInterfaceRegisterCustomAction(
        inst,
        name.c_str(),
        &pri::custom_act_wrapper<
            int,
            +[](void* arg, const char* name) {
                auto inst = reinterpret_cast<uintptr_t>(arg);
                return std::make_tuple(actions[inst][name], 0);
            }>,
        nullptr);
    actions[inst][name] = act;
}

inline bool run_cli(uint64_t inst, const std::string& resource_path, const std::string& user_path, bool directly)
{
    return MaaToolkitProjectInterfaceRunCli(
        inst,
        resource_path.c_str(),
        user_path.c_str(),
        directly,
        +[](const char* message, const char* details_json, void* notify_trans_arg) {
            std::ignore = message;
            std::ignore = details_json;
            std::ignore = notify_trans_arg;
        },
        nullptr);
}

}
