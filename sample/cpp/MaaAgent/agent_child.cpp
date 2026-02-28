#include <iostream>

#include "MaaAgentServer/MaaAgentServerAPI.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaToolkit/MaaToolkitAPI.h"

MaaBool ChildCustomRecognitionCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail);

MaaBool ChildCustomActionCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg);

MaaBool ChildCustomActionInnerCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg);

int main(int argc, char** argv)
{
    std::string user_path = "./";
    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    MaaAgentServerRegisterCustomRecognition("ChildCustomRecognition", ChildCustomRecognitionCallback, nullptr);
    MaaAgentServerRegisterCustomAction("ChildCustomAction", ChildCustomActionCallback, nullptr);
    MaaAgentServerRegisterCustomAction("ChildCustomActionInner", ChildCustomActionInnerCallback, nullptr);

    // from agent_main.cpp
    const char* identifier = argv[1];
    MaaAgentServerStartUp(identifier);

    MaaAgentServerJoin();

    MaaAgentServerShutDown();

    return 0;
}

MaaBool ChildCustomActionCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    std::cout << "at ChildCustomActionCallback" << std::endl;

    MaaContextRunTask(
        context,
        "ChildCustomActionInner",
        R"({
            "ChildCustomActionInner": {
                "action": "Custom",
                "custom_action": "ChildCustomActionInner"
            }
        })");

    return true;
}

MaaBool ChildCustomActionInnerCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    std::cout << "at ChildCustomActionInnerCallback" << std::endl;

    auto buffer = MaaStringBufferCreate();
    MaaContextGetNodeData(context, node_name, buffer);
    const char* detail = MaaStringBufferGet(buffer);
    bool b = MaaContextOverridePipeline(context, detail);
    MaaStringBufferDestroy(buffer);
    return true;
}

MaaBool ChildCustomRecognitionCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail)
{
    std::cout << "at ChildCustomRecognitionCallback" << std::endl;

    return true;
}
