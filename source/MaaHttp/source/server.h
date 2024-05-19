#pragma once

#include "utils/phony.hpp"

void setup_server(
    unsigned short port,
    lhg::server::Server*& server,
    lhg::server::Dispatcher*& dispatcher);

void bind_server_controller(lhg::server::Dispatcher*& dispatcher);
void bind_server_resource(lhg::server::Dispatcher*& dispatcher);
void bind_server_syncctx(lhg::server::Dispatcher*& dispatcher);
void bind_server_buffer(lhg::server::Dispatcher*& dispatcher);
void bind_server_toolkit(lhg::server::Dispatcher*& dispatcher);
void bind_server_rest(lhg::server::Dispatcher*& dispatcher);
void bind_server_misc(lhg::server::Dispatcher*& dispatcher);
