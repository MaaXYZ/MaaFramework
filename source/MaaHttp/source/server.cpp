#include "server.h"
#include "spec/spec.hpp"

void setup_server(
    unsigned short port,
    lhg::server::Server*& server,
    lhg::server::Dispatcher*& dispatcher)
{
    dispatcher = new lhg::server::Dispatcher();
    server = new lhg::server::Server(dispatcher, port);

    lhg::server::apply_function<lhg::maa::__function_list>(dispatcher);
    lhg::server::apply_callback<lhg::maa::__callback_list>(dispatcher);
    lhg::server::apply_handle<lhg::maa::__handle_list>(dispatcher);

    dispatcher->setup_help("/help", "maa http", "1.0.0");
}
