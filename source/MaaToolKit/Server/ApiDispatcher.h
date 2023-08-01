#pragma once

#include "RequestResponse.h"
#include "Utils/Boost.hpp"
#include "Utils/SingletonHolder.hpp"

#include <functional>
#include <meojson/json.hpp>

MAA_TOOLKIT_NS_BEGIN

class ApiDispatcher : public SingletonHolder<ApiDispatcher>
{
public:
    friend class SingletonHolder<ApiDispatcher>;

    using Callback = std::function<void(const json::object&)>;
    using RouteEndpoint = std::function<std::optional<json::object>(json::object, Callback)>;

    void register_route(const std::string& name, RouteEndpoint endpoint);
    std::optional<json::object> handle_route(json::object request, Callback callback);

    void init();

private:
    bool inited = false;

    std::map<std::string, RouteEndpoint> endpoints;
};

MAA_TOOLKIT_NS_END
