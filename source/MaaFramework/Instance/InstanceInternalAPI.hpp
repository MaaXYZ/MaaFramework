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
MAA_NS_END

MAA_VISION_NS_BEGIN
class CustomRecognizer;
using CustomRecognizerPtr = std::shared_ptr<CustomRecognizer>;
MAA_VISION_NS_END

MAA_TASK_NS_BEGIN
class CustomAction;
using CustomActionPtr = std::shared_ptr<CustomAction>;
MAA_TASK_NS_END

MAA_NS_BEGIN

struct InstanceInternalAPI : public NonCopyable
{
public:
    virtual MAA_RES_NS::ResourceMgr* inter_resource() = 0;
    virtual MAA_CTRL_NS::ControllerAgent* inter_controller() = 0;
    virtual InstanceStatus* inter_status() = 0;
    virtual void notify(std::string_view msg, const json::value& details = json::value()) = 0;
    virtual MAA_VISION_NS::CustomRecognizerPtr custom_recognizer(const std::string& name) = 0;
    virtual MAA_TASK_NS::CustomActionPtr custom_action(const std::string& name) = 0;
};

MAA_NS_END
