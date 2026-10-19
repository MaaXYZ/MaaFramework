#include "transport/SocketPlatform.h"
#include "MaaUtils/Platform.h"
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace maa::ios::transport
{
bool initialize_sockets(std::string& err)
{
#ifdef _WIN32
    struct Runtime
    {
        int result;

        Runtime()
        {
            WSADATA data { };
            result = WSAStartup(MAKEWORD(2, 2), &data);
        }

        ~Runtime()
        {
            if (result == 0) {
                WSACleanup();
            }
        }
    };

    static Runtime runtime;
    if (runtime.result != 0) {
        err = "WSAStartup: " + socket_error_message(runtime.result);
        return false;
    }
#else
    (void)err;
#endif
    return true;
}

int socket_error()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string socket_error_message(int code)
{
#ifdef _WIN32
    // 系统消息先取 UTF-16，再转 UTF-8，避免中文 Windows 输出混用代码页。
    wchar_t text[1024] { };
    const DWORD size = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, text, 1024, nullptr);
    auto result = MAA_NS::from_osstring({ text, size });
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
        result.pop_back();
    }
    return result.empty() ? "Winsock error " + std::to_string(code) : result;
#else
    return std::strerror(code);
#endif
}

bool socket_read_timed_out(int code)
{
#ifdef _WIN32
    return code == WSAEWOULDBLOCK || code == WSAETIMEDOUT;
#else
    return code == EAGAIN || code == EWOULDBLOCK;
#endif
}

bool socket_connect_pending(int code)
{
#ifdef _WIN32
    return code == WSAEWOULDBLOCK || code == WSAEINPROGRESS;
#else
    return code == EINPROGRESS;
#endif
}

void close_socket(NativeSocket socket)
{
#ifdef _WIN32
    ::closesocket(socket);
#else
    ::close(socket);
#endif
}

int wait_socket(NativeSocket socket, bool writing, int timeout_ms)
{
#ifdef _WIN32
    fd_set ready, failed;
    FD_ZERO(&ready);
    FD_SET(socket, &ready);
    FD_ZERO(&failed);
    FD_SET(socket, &failed);
    timeval timeout { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    return ::select(0, writing ? nullptr : &ready, writing ? &ready : nullptr, &failed, timeout_ms < 0 ? nullptr : &timeout);
#else
    pollfd fd { socket, static_cast<short>(writing ? POLLOUT : POLLIN), 0 };
    return ::poll(&fd, 1, timeout_ms);
#endif
}
} // namespace maa::ios::transport
