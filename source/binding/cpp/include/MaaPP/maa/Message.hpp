// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

#endif

namespace maa::message
{

MAAPP_EXPORT struct RectRefl : public MaaRect
{
    MEO_JSONIZATION(x, y, width, height);
};

MAAPP_EXPORT struct MessageBase
{
    std::string raw_msg;
    std::string raw_details;

    virtual ~MessageBase() = default;

    void forward(MaaAPICallback callback, MaaTransparentArg arg)
    {
        callback(raw_msg.c_str(), raw_details.c_str(), arg);
    }

    template <typename Msg>
    Msg* is()
    {
        return dynamic_cast<Msg*>(this);
    }
};

MAAPP_EXPORT struct UnknownMessage : public MessageBase
{
};

MAAPP_EXPORT struct InvalidMessage : public MessageBase
{
};

MAAPP_EXPORT struct ResourceMessage : public MessageBase
{
};

MAAPP_EXPORT struct ResourceLoadingMessage : public ResourceMessage
{
    enum
    {
        Started,
        Completed,
        Failed
    } type;

    MaaId id;
    std::string path;

    MEO_JSONIZATION(id, path);
};

MAAPP_EXPORT struct ControllerMessage : public MessageBase
{
};

MAAPP_EXPORT struct ControllerConnectMessage : public ControllerMessage
{
};

MAAPP_EXPORT struct ControllerUUIDGotMessage : public ControllerConnectMessage
{
    std::string uuid;

    MEO_JSONIZATION(uuid);
};

MAAPP_EXPORT struct ControllerUUIDGetFailedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerResolutionGotMessage : public ControllerConnectMessage
{
    struct
    {
        int width;
        int height;
        int orientation;

        MEO_JSONIZATION(width, height, orientation);
    } resolution;

    MEO_JSONIZATION(resolution);
};

MAAPP_EXPORT struct ControllerResolutionGetFailedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerScreencapInitedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerScreencapInitFailedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerTouchInputInitedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerTouchInputInitFailedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerKeyInputInitedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerKeyInputInitFailedMessage : public ControllerConnectMessage
{
};

MAAPP_EXPORT struct ControllerConnectSuccessMessage : public ControllerConnectMessage
{
    std::string uuid;

    struct
    {
        int width;
        int height;
        int orientation;

        MEO_JSONIZATION(width, height, orientation);
    } resolution;

    MEO_JSONIZATION(uuid, resolution);
};

MAAPP_EXPORT struct ControllerConnectFailedMessage : public ControllerConnectMessage
{
    std::string why;

    MEO_JSONIZATION(why);
};

MAAPP_EXPORT struct ControllerActionMessage : public ControllerMessage
{
    enum
    {
        Started,
        Completed,
        Failed
    } type;

    MaaId id;

    MEO_JSONIZATION(id);
};

MAAPP_EXPORT struct InstanceMessage : public MessageBase
{
};

MAAPP_EXPORT struct InstanceTaskMessage : public InstanceMessage
{
    enum
    {
        Started,
        Completed,
        Failed
    } type;

    MaaId id;

    MEO_JSONIZATION(id);
};

MAAPP_EXPORT struct InstanceTaskFocusMessage : public InstanceMessage
{
    enum
    {
        ReadyToRun,
        Runout,
        Completed
    } type;

    MaaId task_id;
    std::string entry;
    std::string uuid;
    std::string hash;
    std::string pre_hit_task;
    std::string name;

    struct
    {
        MaaRecoId id;
        RectRefl box;
        std::string detail;
        bool hit;

        MEO_JSONIZATION(id, box, detail, hit);
    } recognition;

    MaaRunningId running_id;
    MaaStatus status;

    MEO_JSONIZATION(
        task_id,
        entry,
        uuid,
        hash,
        pre_hit_task,
        name,
        recognition,
        running_id,
        status);
};

MAAPP_EXPORT struct InstanceDebugMessage : public InstanceMessage
{
    MaaId task_id;
    std::string entry;
    std::string uuid;
    std::string hash;
    std::string pre_hit_task;
};

MAAPP_EXPORT struct InstanceDebugTaskMessage : public InstanceDebugMessage
{
    enum
    {
        ReadyToRun,
        Runout,
        Completed
    } type;

    std::string name;

    struct
    {
        MaaRecoId id;
        RectRefl box;
        std::string detail;
        bool hit;

        MEO_JSONIZATION(id, box, detail, hit);
    } recognition;

    MaaRunningId running_id;
    MaaStatus status;

    MEO_JSONIZATION(
        task_id,
        entry,
        uuid,
        hash,
        pre_hit_task,
        name,
        recognition,
        running_id,
        status);
};

MAAPP_EXPORT struct InstanceDebugListToRecognizeMessage : public InstanceDebugMessage
{
    std::vector<std::string> list;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, list);
};

MAAPP_EXPORT struct InstanceDebugRecognitionResultMessage : public InstanceDebugMessage
{
    std::string name;

    struct
    {
        MaaRecoId id;
        RectRefl box;
        std::string detail;
        bool hit;

        MEO_JSONIZATION(id, box, detail, hit);
    } recognition;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, name, recognition);
};

MAAPP_EXPORT struct InstanceDebugHitMessage : public InstanceDebugMessage
{
    std::string name;

    struct
    {
        MaaId id;
        RectRefl box;
        std::string detail;
        bool hit;

        MEO_JSONIZATION(id, box, detail, hit);
    } recognition;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, name, recognition);
};

MAAPP_EXPORT struct InstanceDebugMissAllMessage : public InstanceDebugMessage
{
    std::vector<std::string> list;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, list);
};

}
