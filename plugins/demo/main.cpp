#include <MaaFramework/MaaAPI.h>
#include <iostream>
#include <meojson/json.hpp>

struct repeatLaunchParam
{
    std::string task;
    json::object diff;
    int repeat;

    MEO_JSONIZATION(task, MEO_OPT diff, repeat);
};

int maafw_plugin_init(MaaPluginHookDispatcher* hook)
{
    if (hook->version != MaaPluginHookVersion_V0) {
        return 0;
    }
    auto hook_v0 = reinterpret_cast<MaaPluginHook_V0*>(hook);
    hook_v0->on_instance_created = [](MaaInstanceHandle inst) {
        std::cout << "on_instance_created called! instance " << inst << std::endl;
        static MaaCustomActionAPI repeatLaunch = {
            []([[maybe_unused]] MaaSyncContextHandle sync_context,
               [[maybe_unused]] MaaStringView task_name,
               MaaStringView custom_action_param,
               [[maybe_unused]] MaaRectHandle cur_box,
               [[maybe_unused]] MaaStringView cur_rec_detail,
               [[maybe_unused]] MaaTransparentArg action_arg) -> MaaBool {
                auto json_param = json::parse(custom_action_param);
                if (!json_param) {
                    return false;
                }
                repeatLaunchParam param;
                if (!param.from_json(json_param.value())) {
                    return false;
                }
                while (param.repeat-- > 0) {
                    if (!MaaSyncContextRunTask(
                            sync_context,
                            param.task.c_str(),
                            param.diff.to_string().c_str())) {
                        return false;
                    }
                }
                return true;
            },
            [](auto) {
            }
        };
        MaaRegisterCustomAction(inst, "maa.repeat-launch", &repeatLaunch, nullptr);
    };
    hook_v0->on_instance_destroyed = [](MaaInstanceHandle inst) {
        std::cout << "on_instance_destroyed called! instance " << inst << std::endl;
    };
    return 1;
}
