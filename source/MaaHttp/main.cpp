#include <iostream>

#include "utils/phony.hpp"

#include "spec/spec.hpp"

int main()
{
    auto* dispatcher = new lhg::server::Dispatcher();
    auto* server = new lhg::server::Server(dispatcher, 13126);

    lhg::server::apply_function<lhg::maa::__function_list>(dispatcher);
    lhg::server::apply_callback<lhg::maa::__callback_list>(dispatcher);

    server->sync_run();

    return 0;
}
