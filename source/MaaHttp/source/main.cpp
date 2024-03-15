#include <cstdlib>
#include <iostream>

#include "server.h"

int main(int argc, char* argv[])
{
    unsigned port = 13126;
    for (int i = 1; i + 1 < argc; i++) {
        if (!strcmp(argv[i], "-p")) {
            port = atoi(argv[i + 1]);
            if (port == 0 || port > 65535) {
                std::cerr << "invalid port " << argv[i] << std::endl;
                port = 13126;
            }
        }
    }

    lhg::server::Dispatcher* dispatcher;
    lhg::server::Server* server;
    setup_server(static_cast<unsigned short>(port), server, dispatcher);

    server->run();

    std::cerr << "server started at 127.0.0.1:" << port << std::endl;
    std::cerr << "press enter to stop" << std::endl;

    std::cin.get();

    server->stop();

    return 0;
}
