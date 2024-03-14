#include <iostream>

#include "utils/base64.hpp"
#include "utils/phony.hpp"

#include "spec/spec.hpp"

int main()
{
    auto* dispatcher = new lhg::server::Dispatcher();
    auto* server = new lhg::server::Server(dispatcher, 13126);

    lhg::server::apply_function<lhg::maa::__function_list>(dispatcher);
    lhg::server::apply_callback<lhg::maa::__callback_list>(dispatcher);

    dispatcher->setup_help("/help", "maa http", "1.0.0");

    server->sync_run();

    return 0;
}
