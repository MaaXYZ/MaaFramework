#include <iostream>
#include <string>

#include "MaaAgentClient/MaaAgentClientAPI.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

#include "../conf.h"

MaaController* create_adb_controller();

int main()
{
    std::string user_path = "./";
    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    auto controller_handle = create_adb_controller();
    // auto controller_handle = create_win32_controller();
    auto ctrl_id = MaaControllerPostConnection(controller_handle);

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    std::string resource_dir = R"(E:/Code/MaaFramework/sample/resource)";
    auto res_id = MaaResourcePostBundle(resource_handle, resource_dir.c_str());

    MaaControllerWait(controller_handle, ctrl_id);
    MaaResourceWait(resource_handle, res_id);

    auto tasker_handle = MaaTaskerCreate(nullptr, nullptr);
    MaaTaskerBindResource(tasker_handle, resource_handle);
    MaaTaskerBindController(tasker_handle, controller_handle);

    auto destroy = [&]() {
        MaaTaskerDestroy(tasker_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    };

    if (!MaaTaskerInited(tasker_handle)) {
        std::cout << "Failed to init MAA" << std::endl;

        destroy();
        return -1;
    }

    MaaAgentClient* client = MaaAgentClientCreateV2(nullptr);
    MaaAgentClientBindResource(client, resource_handle);

    MaaStringBuffer* identifier = MaaStringBufferCreate();
    MaaAgentClientIdentifier(client, identifier);

    // std::string() + "python E:/Code/MaaFramework/sample/python/MaaAgent/agent_child.py " + MaaStringBufferGet(identifier);
    std::string child_command = std::string() + "E:/Code/MaaFramework/build/bin/Debug/agent_child " + MaaStringBufferGet(identifier);
#ifdef _WIN32
    child_command = "start /b " + child_command;
#else
    child_command = child_command + " &";
#endif
    std::system(child_command.c_str());

    MaaAgentClientConnect(client);

    // ChildCustomTask is defined in agent_child.cpp
    auto task_id = MaaTaskerPostTask(tasker_handle, "MyChildTask", R"(
{
    "MyChildTask": {
        "recognition": "Custom",
        "custom_recognition": "ChildCustomRecognition",
        "action": "Custom",
        "custom_action": "ChildCustomAction"
    }
})");
    MaaTaskerWait(tasker_handle, task_id);

    MaaAgentClientDisconnect(client);

    MaaStringBufferDestroy(identifier);
    MaaAgentClientDestroy(client);
    destroy();

    return 0;
}

MaaController* create_adb_controller()
{
    auto list_handle = MaaToolkitAdbDeviceListCreate();
    auto destroy = [&]() {
        MaaToolkitAdbDeviceListDestroy(list_handle);
    };

    MaaToolkitAdbDeviceFind(list_handle);

    size_t size = MaaToolkitAdbDeviceListSize(list_handle);
    if (size == 0) {
        std::cout << "No device found" << std::endl;

        destroy();
        return nullptr;
    }

    const int kIndex = 0; // for demo, we just use the first device
    auto device_handle = MaaToolkitAdbDeviceListAt(list_handle, kIndex);

    std::string agent_path = "share/MaaAgentBinary";
    auto controller_handle = MaaAdbControllerCreate(
        MaaToolkitAdbDeviceGetAdbPath(device_handle),
        MaaToolkitAdbDeviceGetAddress(device_handle),
        MaaToolkitAdbDeviceGetScreencapMethods(device_handle),
        MaaToolkitAdbDeviceGetInputMethods(device_handle),
        MaaToolkitAdbDeviceGetConfig(device_handle),
        agent_path.c_str(),
        nullptr,
        nullptr);

    destroy();

    return controller_handle;
}
