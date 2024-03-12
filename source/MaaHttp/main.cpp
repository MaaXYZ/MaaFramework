#include <iostream>

#include "info.hpp"
#include "server/dispatcher.hpp"
#include "server/server.hpp"
#include "utils/phony.hpp"

#include "specialize.hpp"

int main()
{
    auto* dispatcher = new lhg::server::Dispatcher();
    auto* server = new lhg::server::Server(dispatcher, 13126);

    lhg::server::apply_function<lhg::maa::__function_list>(dispatcher);

    // dispatcher->handle(
    //     "/api/MaaVersion",
    //     [](auto& provider, auto& res, const auto& req) {
    //         lhg::call::call<lhg::maa::function_MaaVersion>(provider, res, req);
    //     },
    //     [](auto& provider, auto& res) {});

    server->sync_run();

    return 0;

    // json::object res, req;
    // lhg::ManagerProvider provider;

    // lhg::call::call<lhg::maa::function_MaaSetGlobalOptionString>(
    //     provider, res, { { "key", MaaGlobalOption_LogDir }, { "value", "log" } });
    // std::cout << res.format() << std::endl;

    // res = {};
    // lhg::call::call<lhg::maa::function_MaaToolkitPostFindDevice>(provider, res, req);
    // std::cout << res.format() << std::endl;

    // res = {};
    // lhg::call::call<lhg::maa::function_MaaToolkitWaitForFindDeviceToComplete>(provider, res, req);
    // std::cout << res.format() << std::endl;

    // auto cb = lhg::callback::create_callback<lhg::maa::callback_MaaAPICallback>();
    // lhg::callback::context_info context { &provider,
    //                                       [](auto& res, const auto& req, auto ctx) {
    //                                           std::cout << req.format() << std::endl;
    //                                           std::ignore = res;
    //                                           return;
    //                                       },
    //                                       nullptr };
    // cb("hello", "world!", &context);
}
