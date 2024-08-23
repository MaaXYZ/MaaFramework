#include <MaaFramework/MaaAPI.h>

#include <iostream>

int maafw_plugin_init(MaaPluginHookDispatcher* hook)
{
    if (hook->version != MaaPluginHookVersion_V0) {
        return 0;
    }
    auto hook_v0 = reinterpret_cast<MaaPluginHook_V0*>(hook);
    hook_v0->on_instance_created = [](MaaInstanceHandle inst) {
        std::cout << "on_instance_created called! instance " << inst << std::endl;
    };
    hook_v0->on_instance_destroyed = [](MaaInstanceHandle inst) {
        std::cout << "on_instance_destroyed called! instance " << inst << std::endl;
    };
    return 1;
}
