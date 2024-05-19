#include "server.h"

void setup_server(
    unsigned short port,
    lhg::server::Server*& server,
    lhg::server::Dispatcher*& dispatcher)
{
    dispatcher = new lhg::server::Dispatcher();
    server = new lhg::server::Server(dispatcher, port);

    bind_server_controller(dispatcher);
    bind_server_resource(dispatcher);
    bind_server_syncctx(dispatcher);
    bind_server_buffer(dispatcher);
    bind_server_toolkit(dispatcher);
    bind_server_rest(dispatcher);
    bind_server_misc(dispatcher);

    dispatcher->setup_help("/help", "maa http", "1.0.0");
}
