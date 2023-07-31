#pragma once

#include "../ApiDispatcher.h"
#include "MaaFramework/MaaAPI.h"

MAA_TOOLKIT_NS_BEGIN

void init_maa_framework(ApiDispatcher& disp)
{
    disp.register_route(ApiDispatcher::Method::get, "/api/version", [](ApiDispatcher::Params, ApiDispatcher::Body) {
        return json::object { { "version", MaaVersion() } };
    });
}

MAA_TOOLKIT_NS_END
