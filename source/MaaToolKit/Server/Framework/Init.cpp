#include "Init.h"

#include "Controller.h"
#include "MaaFramework/MaaAPI.h"

MAA_TOOLKIT_SERVER_NS_BEGIN

void init_maa_framework(ApiDispatcher& disp)
{
    disp.register_route("framework.version", [](json::object) -> std::optional<json::object> {
        return json::object { { "version", MaaVersion() } };
    });

    init_maa_framework_controller(disp);
}

MAA_TOOLKIT_SERVER_NS_END
