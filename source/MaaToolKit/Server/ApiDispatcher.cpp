#include "ApiDispatcher.h"

MAA_TOOLKIT_NS_BEGIN

void ApiDispatcher::registerRoute(Method method, const std::string& path, RouteEndpoint endpoint)
{
    endpoints[method][path] = endpoint;
}

json::object ApiDispatcher::handleRoute(boost::beast::http::request<boost::beast::http::string_body>&& request)
{
    auto url = boost::urls::parse_origin_form(request.target()).value();
    // endpoints[request.method()][url.path()]
    if (!endpoints.count(request.method())) {
        return { { "error", "Unknown method" } };
    }
    auto& epm = endpoints[request.method()];
    if (!epm.count(url.path())) {
        return { { "error", "Unknown path" } };
    }
    return epm[url.path()](url.params(), request.body());
}

MAA_TOOLKIT_NS_END
