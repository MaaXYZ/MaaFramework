// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

namespace maa::message
{

struct RectRefl : public MaaRect
{
    MEO_JSONIZATION(x, y, width, height);
};

struct MessageBase
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

struct UnknownMessage : public MessageBase
{
};

struct InvalidMessage : public MessageBase
{
};

struct ResourceMessage : public MessageBase
{
};

struct ResourceLoadingMessage : public ResourceMessage
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

struct ControllerMessage : public MessageBase
{
};

struct ControllerConnectMessage : public ControllerMessage
{
};

struct ControllerUUIDGotMessage : public ControllerConnectMessage
{
    std::string uuid;

    MEO_JSONIZATION(uuid);
};

struct ControllerUUIDGetFailedMessage : public ControllerConnectMessage
{
};

struct ControllerResolutionGotMessage : public ControllerConnectMessage
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

struct ControllerResolutionGetFailedMessage : public ControllerConnectMessage
{
};

struct ControllerScreencapInitedMessage : public ControllerConnectMessage
{
};

struct ControllerScreencapInitFailedMessage : public ControllerConnectMessage
{
};

struct ControllerTouchInputInitedMessage : public ControllerConnectMessage
{
};

struct ControllerTouchInputInitFailedMessage : public ControllerConnectMessage
{
};

struct ControllerKeyInputInitedMessage : public ControllerConnectMessage
{
};

struct ControllerKeyInputInitFailedMessage : public ControllerConnectMessage
{
};

struct ControllerConnectSuccessMessage : public ControllerConnectMessage
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

struct ControllerConnectFailedMessage : public ControllerConnectMessage
{
    std::string why;

    MEO_JSONIZATION(why);
};

struct ControllerActionMessage : public ControllerMessage
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

struct InstanceMessage : public MessageBase
{
};

struct InstanceTaskMessage : public InstanceMessage
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

struct InstanceTaskFocusMessage : public InstanceMessage
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

    MaaNodeId node_id;
    MaaStatus status;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, name, recognition, node_id, status);
};

struct InstanceDebugMessage : public InstanceMessage
{
    MaaId task_id;
    std::string entry;
    std::string uuid;
    std::string hash;
    std::string pre_hit_task;
};

struct InstanceDebugTaskMessage : public InstanceDebugMessage
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

    MaaNodeId node_id;
    MaaStatus status;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, name, recognition, node_id, status);
};

struct InstanceDebugListToRecognizeMessage : public InstanceDebugMessage
{
    std::vector<std::string> list;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, list);
};

struct InstanceDebugRecognitionResultMessage : public InstanceDebugMessage
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

struct InstanceDebugHitMessage : public InstanceDebugMessage
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

struct InstanceDebugMissAllMessage : public InstanceDebugMessage
{
    std::vector<std::string> list;

    MEO_JSONIZATION(task_id, entry, uuid, hash, pre_hit_task, list);
};

}
