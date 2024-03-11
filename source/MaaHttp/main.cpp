#include <iostream>

#include "utils/phony.hpp"
#include "info.hpp"

int main()
{
    json::object res, req;
    lhg::call<lhg::maa::function_MaaVersion>(res, req);
    std::cout << res.format() << std::endl;
}
