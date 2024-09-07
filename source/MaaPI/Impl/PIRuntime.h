#pragma once

#include "Conf/Conf.h"
#include "Impl/PIConfig.h"
#include "Impl/PIData.h"
#include "MaaFramework/MaaDef.h"
#include "PI/Types.h"

MAA_PI_NS_BEGIN

class PIRuntime : public MaaPIRuntime
{
    friend class PIClient;

public:
    virtual bool bind(MaaPIData* data, MaaPIConfig* config) override;
    virtual bool setup(MaaNotificationCallback cb, void* cb_arg) override;
    virtual MaaController* get_controller() override;
    virtual MaaResource* get_resource() override;
    virtual MaaTasker* get_tasker() override;

private:
    PIData* data_;
    PIConfig* config_;
    MaaController* controller_;
    MaaResource* resource_;
    MaaTasker* tasker_;
};

MAA_PI_NS_END
