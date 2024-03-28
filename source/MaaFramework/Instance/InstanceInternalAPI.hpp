#pragma once

#include <memory>
#include <string>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
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
    MaaCustomRecognizerHandle recognizer_ = nullptr;
    MaaTransparentArg recognizer_arg_ = nullptr;
    InstanceInternalAPI* inst_ = nullptr;
};

struct CustomActionSession
{
    MaaCustomActionHandle action_ = nullptr;
    MaaTransparentArg action_arg_ = nullptr;
    InstanceInternalAPI* inst_ = nullptr;
};

struct InstanceInternalAPI : public NonCopyable
{
public:
    virtual MAA_RES_NS::ResourceMgr* inter_resource() = 0;
    virtual MAA_CTRL_NS::ControllerAgent* inter_controller() = 0;
    virtual InstanceStatus* inter_status() = 0;
    virtual void notify(std::string_view msg, const json::value& details = json::value()) = 0;
    virtual CustomRecognizerSession custom_recognizer(const std::string& name) = 0;
    virtual CustomActionSession custom_action(const std::string& name) = 0;
};

MAA_NS_END
