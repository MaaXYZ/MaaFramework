// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <memory>

#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/maa/Message.hpp"

namespace maa::message
{

namespace details
{

template <typename Msg>
inline Msg* make(MaaStringView msg, MaaStringView details)
{
    auto obj = new Msg();
    obj->raw_msg = msg;
    obj->raw_details = details;
    return obj;
}

}

#define __EMPTY_MSG(m, cls)                        \
    else if (msgstr == m)                          \
    {                                              \
        auto p = details::make<cls>(msg, details); \
        set(p);                                    \
    }

#define __SERIAL_MSG(m, cls)                       \
    else if (msgstr == m)                          \
    {                                              \
        auto p = details::make<cls>(msg, details); \
        if (p->from_json(obj)) {                   \
            set(p);                                \
        }                                          \
    }

#define __SERIAL_MSG_TYPE(m, cls, t)               \
    else if (msgstr == m)                          \
    {                                              \
        auto p = details::make<cls>(msg, details); \
        p->type = cls::t;                          \
        if (p->from_json(obj)) {                   \
            set(p);                                \
        }                                          \
    }

inline std::shared_ptr<MessageBase> parse(MaaStringView msg, MaaStringView details)
{
    auto fail = [&]() {
        return std::shared_ptr<MessageBase>(details::make<UnknownMessage>(msg, details));
    };
    auto detail_opt = json::parse(details);
    if (!detail_opt.has_value() || !detail_opt.value().is_object()) {
        return fail();
    }
    const auto& obj = detail_opt.value().as_object();

    std::shared_ptr<MessageBase> pmsg;
    std::string msgstr = msg;
    auto set = [&pmsg](MessageBase* p) {
        pmsg = std::shared_ptr<MessageBase>(p);
    };

    if (false) {
    }
    __EMPTY_MSG(MaaMsg_Invalid, InvalidMessage)
    __SERIAL_MSG_TYPE(MaaMsg_Resource_StartLoading, ResourceLoadingMessage, Started)
    __SERIAL_MSG_TYPE(MaaMsg_Resource_LoadingCompleted, ResourceLoadingMessage, Completed)
    __SERIAL_MSG_TYPE(MaaMsg_Resource_LoadingFailed, ResourceLoadingMessage, Failed)
    __SERIAL_MSG(MaaMsg_Controller_UUIDGot, ControllerUUIDGotMessage)
    __EMPTY_MSG(MaaMsg_Controller_UUIDGetFailed, ControllerUUIDGetFailedMessage)
    __SERIAL_MSG(MaaMsg_Controller_ResolutionGot, ControllerResolutionGotMessage)
    __EMPTY_MSG(MaaMsg_Controller_ResolutionGetFailed, ControllerResolutionGetFailedMessage)
    __EMPTY_MSG(MaaMsg_Controller_ScreencapInited, ControllerScreencapInitedMessage)
    __EMPTY_MSG(MaaMsg_Controller_ScreencapInitFailed, ControllerScreencapInitFailedMessage)
    __EMPTY_MSG(MaaMsg_Controller_TouchInputInited, ControllerTouchInputInitedMessage)
    __EMPTY_MSG(MaaMsg_Controller_TouchInputInitFailed, ControllerTouchInputInitFailedMessage)
    __EMPTY_MSG(MaaMsg_Controller_KeyInputInited, ControllerKeyInputInitedMessage)
    __EMPTY_MSG(MaaMsg_Controller_KeyInputInitFailed, ControllerKeyInputInitFailedMessage)
    __SERIAL_MSG(MaaMsg_Controller_ConnectSuccess, ControllerConnectSuccessMessage)
    __SERIAL_MSG(MaaMsg_Controller_ConnectFailed, ControllerConnectFailedMessage)
    __SERIAL_MSG_TYPE(MaaMsg_Controller_Action_Started, ControllerActionMessage, Started)
    __SERIAL_MSG_TYPE(MaaMsg_Controller_Action_Completed, ControllerActionMessage, Completed)
    __SERIAL_MSG_TYPE(MaaMsg_Controller_Action_Failed, ControllerActionMessage, Failed)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Started, InstanceTaskMessage, Started)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Completed, InstanceTaskMessage, Completed)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Failed, InstanceTaskMessage, Failed)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Focus_ReadyToRun, InstanceTaskFocusMessage, ReadyToRun)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Focus_Runout, InstanceTaskFocusMessage, Runout)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Focus_Completed, InstanceTaskFocusMessage, Completed)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Debug_ReadyToRun, InstanceDebugTaskMessage, ReadyToRun)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Debug_Runout, InstanceDebugTaskMessage, Runout)
    __SERIAL_MSG_TYPE(MaaMsg_Task_Debug_Completed, InstanceDebugTaskMessage, Completed)
    __SERIAL_MSG(MaaMsg_Task_Debug_ListToRecognize, InstanceDebugListToRecognizeMessage)
    __SERIAL_MSG(MaaMsg_Task_Debug_RecognitionResult, InstanceDebugRecognitionResultMessage)
    __SERIAL_MSG(MaaMsg_Task_Debug_Hit, InstanceDebugHitMessage)
    __SERIAL_MSG(MaaMsg_Task_Debug_MissAll, InstanceDebugMissAllMessage)

    if (!pmsg) {
        return fail();
    }
    return pmsg;
}

#undef __EMPTY_MSG
#undef __SERIAL_MSG
#undef __SERIAL_MSG_TYPE

}
