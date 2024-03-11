#include <iostream>

#include "MaaFramework/MaaDef.h"
#include "info.hpp"
#include "manager/manager.hpp"
#include "utils/phony.hpp"

int main()
{
    json::object res, req;
    lhg::ManagerProvider provider;

    lhg::call<lhg::maa::function_MaaSetGlobalOptionString>(provider, res,
                                                           { { "key", MaaGlobalOption_LogDir }, { "value", "log" } });
    std::cout << res.format() << std::endl;
    
    res = {};
    lhg::call<lhg::maa::function_MaaToolkitPostFindDevice>(provider, res, req);
    std::cout << res.format() << std::endl;
    
    res = {};
    lhg::call<lhg::maa::function_MaaToolkitWaitForFindDeviceToComplete>(provider, res, req);
    std::cout << res.format() << std::endl;
}
