#pragma once

#include "utils/phony.hpp"

extern void setup_server(
    unsigned short port,
    lhg::server::Server*& server,
    lhg::server::Dispatcher*& dispatcher);
