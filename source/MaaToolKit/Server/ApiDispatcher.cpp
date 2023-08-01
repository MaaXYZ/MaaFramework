#include "ApiDispatcher.h"
#include "Framework/Init.h"
#include "JsonValidator.hpp"

MAA_TOOLKIT_NS_BEGIN

void ApiDispatcher::register_route(const std::string& name, RouteEndpoint endpoint)
{
    endpoints[name] = endpoint;
}

std::optional<json::object> ApiDispatcher::handle_route(json::object request, Callback callback)
{
    auto action = require_key_as_string(request, "action");
    if (!action.has_value()) {
        return std::nullopt;
    }
    if (!endpoints.count(action.value())) {
        return std::nullopt;
    }

    auto param = require_key(request, "param").value_or(json::object {});
    if (!param.is_object()) {
        return std::nullopt;
    }

    return endpoints[action.value()](param.as_object(), callback);
}

void ApiDispatcher::init()
{
    if (inited) {
        return;
    }
    init_maa_framework(*this);
    inited = true;
}

MAA_TOOLKIT_NS_END
