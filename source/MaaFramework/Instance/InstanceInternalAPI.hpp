#pragma once

#include <memory>
#include <string>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/Task/MaaCustomAction.h"
#include "MaaFramework/Task/MaaCustomRecognizer.h"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN
class ResourceMgr;
MAA_RES_NS_END

MAA_CTRL_NS_BEGIN
class ControllerAgent;
MAA_CTRL_NS_END

MAA_NS_BEGIN

class InstanceStatus;

struct CustomRecognizerSession
{
    MaaCustomRecognizerHandle recognizer = nullptr;
    MaaTransparentArg recognizer_arg = nullptr;
};

struct CustomActionSession
{
    MaaCustomActionHandle action = nullptr;
    MaaTransparentArg action_arg = nullptr;
};

struct InstanceInternalAPI : public NonCopyable
{
public:
    virtual MAA_RES_NS::ResourceMgr* inter_resource() = 0;
    virtual MAA_CTRL_NS::ControllerAgent* inter_controller() = 0;
    virtual void notify(std::string_view msg, const json::value& details = json::value()) = 0;
    virtual CustomRecognizerSession* custom_recognizer_session(const std::string& name) = 0;
    virtual CustomActionSession* custom_action_session(const std::string& name) = 0;
};

MAA_NS_END