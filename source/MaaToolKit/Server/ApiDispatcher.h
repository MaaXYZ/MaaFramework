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

    using Method = boost::beast::http::verb;

    using RouteEndpoint = std::function<void(RequestResponse&)>;

    void register_route(Method method, const std::string& path, RouteEndpoint endpoint);
    void handle_route(RequestResponse& rr);

private:
    std::map<Method, std::map<std::string, RouteEndpoint>> endpoints;
};

MAA_TOOLKIT_NS_END
