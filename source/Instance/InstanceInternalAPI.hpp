#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"
#include "Instance/InstanceStatus.h"

MAA_RES_NS_BEGIN
class ResourceMgr;
MAA_RES_NS_END

MAA_CTRL_NS_BEGIN
class ControllerMgr;
MAA_CTRL_NS_END

MAA_NS_BEGIN

struct InstanceInternalAPI : public NonCopyable
{
public:
    virtual MAA_RES_NS::ResourceMgr* resource() = 0;
    virtual MAA_CTRL_NS::ControllerMgr* controller() = 0;
    virtual InstanceStatus* status() = 0;
};

MAA_NS_END
