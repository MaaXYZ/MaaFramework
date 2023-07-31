#pragma once

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
    using Params = boost::urls::params_view;
    using Body = std::string_view;

    using RouteEndpoint = std::function<json::object(Params param, Body body)>;

    void register_route(Method method, const std::string& path, RouteEndpoint endpoint);
    json::object handle_route(boost::beast::http::request<boost::beast::http::string_body>&& request);

private:
    std::map<Method, std::map<std::string, RouteEndpoint>> endpoints;
};

MAA_TOOLKIT_NS_END
