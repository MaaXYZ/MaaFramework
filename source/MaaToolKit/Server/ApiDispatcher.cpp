#include "ApiDispatcher.h"

MAA_TOOLKIT_NS_BEGIN

void ApiDispatcher::register_route(Method method, const std::string& path, RouteEndpoint endpoint)
{
    endpoints[method][path] = endpoint;
}

void ApiDispatcher::handle_route(RequestResponse& rr)
{
    const auto& request = rr.get_request();

    auto url = boost::urls::parse_origin_form(request.target()).value();
    if (!endpoints.count(request.method())) {
        rr.reply_error("Unknown method", boost::beast::http::status::bad_request);
        return;
    }
    auto& epm = endpoints[request.method()];
    if (!epm.count(url.path())) {
        rr.reply_error("Unknown path", boost::beast::http::status::bad_request);
        return;
    }
    epm[url.path()](rr, url.params(), request.body());
}

MAA_TOOLKIT_NS_END
