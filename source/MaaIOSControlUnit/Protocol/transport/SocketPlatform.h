#pragma once

#include <string>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socklen_t = int;
#else
#include <cerrno>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#endif

namespace maa::ios::transport
{
#ifdef _WIN32
using NativeSocket = SOCKET;
inline constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;
inline constexpr int kSocketInterrupted = WSAEINTR;
inline constexpr int kShutdownBoth = SD_BOTH;
#else
using NativeSocket = int;
inline constexpr NativeSocket kInvalidSocket = -1;
inline constexpr int kSocketInterrupted = EINTR;
inline constexpr int kShutdownBoth = SHUT_RDWR;
#endif

// 只封装本机 socket API 差异；隧道里的 TCP/UDP 仍由 lwIP 处理。
bool initialize_sockets(std::string& err);
int socket_error();
std::string socket_error_message(int code);

inline std::string socket_error_message()
{
    return socket_error_message(socket_error());
}

bool socket_read_timed_out(int code);
bool socket_connect_pending(int code);
void close_socket(NativeSocket socket);
int wait_socket(NativeSocket socket, bool writing, int timeout_ms);
} // namespace maa::ios::transport
