#include <iostream>

#include "MaaAgentServer/MaaAgentServerAPI.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/Utility/MaaBuffer.h"

#include "../conf.h"

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

MaaStringListBuffer* create_args(int argc, char** argv);

int main(int argc, char** argv)
{
    MaaAgentServerRegisterCustomRecognition("ChildCustomRecognition", ChildCustomRecognitionCallback, nullptr);
    MaaAgentServerRegisterCustomAction("ChildCustomAction", ChildCustomActionCallback, nullptr);
    MaaAgentServerRegisterCustomAction("ChildCustomActionInner", ChildCustomActionInnerCallback, nullptr);

    MaaStringListBuffer* args = create_args(argc, argv);

    MaaAgentServerStartUp(args);

    MaaStringListBufferDestroy(args);

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

MaaStringListBuffer* create_args(int argc, char** argv)
{
    MaaStringListBuffer* args = MaaStringListBufferCreate();
    for (int i = 1; i < argc; ++i) {
        MaaStringBuffer* arg = MaaStringBufferCreate();
        MaaStringBufferSet(arg, argv[i]);
        MaaStringListBufferAppend(args, arg);
        MaaStringBufferDestroy(arg);
    }
    return args;
}
