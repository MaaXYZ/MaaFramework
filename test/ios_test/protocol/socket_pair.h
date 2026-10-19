#pragma once
#include "transport/TcpConnect.h"
#ifndef _WIN32
#include <sys/socket.h>
#endif

inline bool make_test_socket_pair(maa::ios::transport::Socket& a, maa::ios::transport::Socket& b, std::string& err)
{
    using namespace maa::ios::transport;
#ifdef _WIN32
    if (!initialize_sockets(err)) {
        return false;
    }
    Socket listener(::socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in address { };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t size = sizeof(address);
    if (!listener.valid() || ::bind(listener.fd(), reinterpret_cast<sockaddr*>(&address), size) != 0
        || ::getsockname(listener.fd(), reinterpret_cast<sockaddr*>(&address), &size) != 0 || ::listen(listener.fd(), 1) != 0) {
        err = socket_error_message();
        return false;
    }
    auto client = connect_tcp("127.0.0.1", ntohs(address.sin_port), 1000, err);
    if (!client) {
        return false;
    }
    b.reset(::accept(listener.fd(), nullptr, nullptr));
    if (!b.valid()) {
        err = socket_error_message();
        return false;
    }
    a = std::move(*client);
    return true;
#else
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        err = socket_error_message();
        return false;
    }
    a.reset(fds[0]);
    b.reset(fds[1]);
    return true;
#endif
}
