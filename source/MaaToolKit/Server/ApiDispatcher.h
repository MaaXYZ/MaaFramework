#pragma once

#include "RequestResponse.h"
#include "Utils/Boost.hpp"
#include "Utils/SingletonHolder.hpp"

#include <functional>
#include <meojson/json.hpp>

MAA_TOOLKIT_SERVER_NS_BEGIN

class ApiDispatcher : public SingletonHolder<ApiDispatcher>
{
public:
    friend class SingletonHolder<ApiDispatcher>;

    using RouteEndpoint = std::function<std::optional<json::object>(json::object)>;

    void register_route(const std::string& name, RouteEndpoint endpoint);
    std::optional<json::object> handle_route(json::object request);

    void init();

private:
    bool inited = false;

    std::map<std::string, RouteEndpoint> endpoints;
};

MAA_TOOLKIT_SERVER_NS_END
