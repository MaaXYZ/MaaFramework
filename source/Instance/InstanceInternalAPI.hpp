#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"
#include "Instance/InstanceStatus.h"

struct MaaResourceAPI;
struct MaaControllerAPI;

MAA_NS_BEGIN

struct InstanceInternalAPI : public NonCopyable
{
public:
    virtual MaaResourceAPI* resource() = 0;
    virtual MaaControllerAPI* controller() = 0;
    virtual InstanceStatus* status() = 0;
};

MAA_NS_END
