#include <iostream>

#include "MaaFramework/MaaDef.h"
#include "callback/callback.hpp"
#include "info.hpp"
#include "manager/manager.hpp"
#include "utils/phony.hpp"

int main()
{
    // json::object res, req;
    lhg::ManagerProvider provider;

    // lhg::call::call<lhg::maa::function_MaaSetGlobalOptionString>(
    //     provider, res, { { "key", MaaGlobalOption_LogDir }, { "value", "log" } });
    // std::cout << res.format() << std::endl;

    // res = {};
    // lhg::call::call<lhg::maa::function_MaaToolkitPostFindDevice>(provider, res, req);
    // std::cout << res.format() << std::endl;

    // res = {};
    // lhg::call::call<lhg::maa::function_MaaToolkitWaitForFindDeviceToComplete>(provider, res, req);
    // std::cout << res.format() << std::endl;

    auto cb = lhg::callback::create_callback<lhg::maa::callback_MaaAPICallback>();
    lhg::callback::context_info context { &provider,
                                          [](auto& res, const auto& req) {
                                              std::cout << req.format() << std::endl;
                                              std::ignore = res;
                                              return;
                                          },
                                          nullptr };
    cb("hello", "world!", &context);
}
