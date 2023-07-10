#pragma once

#include <memory>
#include <string>

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

MAA_RES_NS_BEGIN
class ResourceMgr;
MAA_RES_NS_END

MAA_CTRL_NS_BEGIN
class ControllerMgr;
MAA_CTRL_NS_END

MAA_NS_BEGIN
class InstanceStatus;
MAA_NS_END

MAA_TASK_NS_BEGIN
class CustomTask;
using CustomTaskPtr = std::shared_ptr<CustomTask>;
MAA_TASK_NS_END

MAA_NS_BEGIN

struct InstanceInternalAPI : public NonCopyable
{
public:
    virtual MAA_RES_NS::ResourceMgr* resource() = 0;
    virtual MAA_CTRL_NS::ControllerMgr* controller() = 0;
    virtual InstanceStatus* status() = 0;
    virtual MAA_TASK_NS::CustomTaskPtr custom_task(const std::string& name) = 0;
};

MAA_NS_END
