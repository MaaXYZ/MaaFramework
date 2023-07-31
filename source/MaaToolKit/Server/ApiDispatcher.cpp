#include "ApiDispatcher.h"
#include "JsonValidator.hpp"

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
        rr.reply_bad_request("unknown method");
        return;
    }
    auto& epm = endpoints[request.method()];
    if (!epm.count(url.path())) {
        rr.reply_bad_request("unknown path");
        return;
    }
    try {
        epm[url.path()](rr);
    }
    catch (JsonValidateFailedException err) {
        rr.reply_bad_request(err.what());
    }
}

MAA_TOOLKIT_NS_END
