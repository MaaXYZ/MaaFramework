#include <array>
#include <atomic>
#include <cstring>
#include <thread>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaPlugin/MaaPluginAPI.h"
#include "fixture.h"

namespace
{
std::array<std::atomic<int>, 13> counts {};

MaaBool MAA_CALL action(MaaContext*, MaaTaskId, const char*, const char*, const char*, MaaRecoId, const MaaRect*, void*)
{
    ++counts[10];
    return true;
}
}

extern "C" MAA_PLUGIN_API int PluginTestCount(int index)
{
    return counts.at(index);
}

const char* GetPluginVersion()
{
    return "test";
}

uint32_t GetApiVersion()
{
    return 1;
}

void OnPluginInit()
{
    ++counts[0];
}

void OnPluginFlush()
{
    ++counts[1];
    std::thread reader([] {
        auto* names = MaaStringListBufferCreate();
        if (MaaGlobalGetPluginList(names)) {
            ++counts[12];
        }
        MaaStringListBufferDestroy(names);
    });
    reader.join();
}

void OnPluginShutdown()
{
    ++counts[2];
}

void OnResourceEvent(void* handle, const char* message, const char*, void*)
{
    if (std::strcmp(message, MaaMsg_Resource_Created) == 0) {
        ++counts[3];
        MaaResourceRegisterCustomAction(static_cast<MaaResource*>(handle), "PluginTestAction", action, nullptr);
    }
    else if (std::strcmp(message, MaaMsg_Resource_Destroying) == 0) {
        ++counts[4];
    }
    else {
        ++counts[9];
    }
}

void OnTaskerEvent(void*, const char* message, const char*, void*)
{
    if (std::strcmp(message, MaaMsg_Tasker_Created) == 0) {
        ++counts[5];
    }
    else if (std::strcmp(message, MaaMsg_Tasker_Destroying) == 0) {
        ++counts[6];
    }
    else {
        ++counts[11];
    }
}

void OnControllerEvent(void*, const char* message, const char*, void*)
{
    if (std::strcmp(message, MaaMsg_Controller_Created) == 0) {
        ++counts[7];
    }
    else if (std::strcmp(message, MaaMsg_Controller_Destroying) == 0) {
        ++counts[8];
    }
}
