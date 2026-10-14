#include "transport/Socket.h"
#include "i18n/Translation.h"
#include <algorithm>
#include <climits>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

namespace maa::ios::transport
{
namespace
{
// Suppress SIGPIPE on this owned socket only; the TLS BIO also uses MSG_NOSIGNAL.
void disable_sigpipe(NativeSocket fd)
{
#ifdef SO_NOSIGPIPE
    const int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#else
    (void)fd;
#endif
}

} // namespace

// ------------------------------------------------------------ Socket ------

Socket::Socket(NativeSocket fd)
    : fd_(fd)
{
    if (valid()) {
        disable_sigpipe(fd_);
    }
}

Socket::Socket(Socket&& other) noexcept
    : fd_(other.fd_)
{
    other.fd_ = kInvalidSocket;
}

Socket& Socket::operator=(Socket&& other) noexcept
{
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = kInvalidSocket;
    }
    return *this;
}

Socket::~Socket()
{
    close();
}

void Socket::reset(NativeSocket fd)
{
    close();
    fd_ = fd;
    if (valid()) {
        disable_sigpipe(fd_);
    }
}

void Socket::close()
{
    if (valid()) {
        close_socket(fd_);
        fd_ = kInvalidSocket;
    }
}

#if defined(MSG_NOSIGNAL)
// Both ordinary writes and the TLS BIO use socket-local SIGPIPE protection.
constexpr int kNoSigPipe = MSG_NOSIGNAL;
#else
constexpr int kNoSigPipe = 0;
#endif

bool Socket::write_all(const void* data, size_t len, std::string& err)
{
    const auto* p = static_cast<const uint8_t*>(data);
    size_t left = len;
    while (left > 0) {
        const auto n = ::send(fd_, reinterpret_cast<const char*>(p), static_cast<int>(std::min(left, size_t(INT_MAX))), kNoSigPipe);
        if (n < 0) {
            if (socket_error() == kSocketInterrupted) {
                continue;
            }
            err = std::string(MAA_IOS_TR("send failed: ")) + socket_error_message();
            return false;
        }
        if (n == 0) {
            err = MAA_IOS_TR("Peer closed write direction");
            return false;
        }
        p += static_cast<size_t>(n);
        left -= static_cast<size_t>(n);
    }
    return true;
}

bool Socket::read_exact(void* data, size_t len, std::string& err)
{
    if (len == 0) {
        return true;
    }
    auto* p = static_cast<uint8_t*>(data);
    size_t got = 0;
    while (got < len) {
        const auto n = ::recv(fd_, reinterpret_cast<char*>(p + got), static_cast<int>(std::min(len - got, size_t(INT_MAX))), 0);
        if (n < 0) {
            if (socket_error() == kSocketInterrupted) {
                continue;
            }
            if (socket_read_timed_out(socket_error())) {
                err = MAA_IOS_TR("Read timed out (no bytes before deadline)");
                return false;
            }
            err = std::string(MAA_IOS_TR("recv failed: ")) + socket_error_message();
            return false;
        }
        if (n == 0) {
            err = MAA_IOS_TR("Peer closed (received only ") + std::to_string(got) + "/" + std::to_string(len) + MAA_IOS_TR(")");
            return false;
        }
        got += static_cast<size_t>(n);
    }
    return true;
}

bool Socket::set_read_timeout(int ms, std::string& err)
{
#ifdef _WIN32
    const DWORD tv = static_cast<DWORD>(std::max(ms, 0));
#else
    timeval tv { };
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
#endif
    if (::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)) != 0) {
        err = std::string(MAA_IOS_TR("Failed to set read timeout: ")) + socket_error_message();
        return false;
    }
    return true;
}

void Socket::interrupt()
{
    if (valid()) {
        ::shutdown(fd_, kShutdownBoth);
#ifdef _WIN32
        // Winsock shutdown 不一定唤醒已挂起的接收；取消该句柄的未完成 I/O，
        // 所有权仍保留到使用它的线程退出后再关闭。
        CancelIoEx(reinterpret_cast<HANDLE>(fd_), nullptr);
#endif
    }
}

bool Socket::wait_readable(int ms, std::string& err, bool* timed_out)
{
    if (timed_out) {
        *timed_out = false;
    }
    if (!valid()) {
        err = MAA_IOS_TR("Socket closed");
        return false;
    }
    for (;;) {
        const int n = wait_socket(fd_, false, ms);
        if (n > 0) {
            return true;
        }
        if (n == 0) {
            if (timed_out) {
                *timed_out = true;
                err.clear();
            }
            else {
                err = MAA_IOS_TR("Wait timed out");
            }
            return false;
        }
        if (socket_error() != kSocketInterrupted) {
            err = std::string(MAA_IOS_TR("poll failed: ")) + socket_error_message();
            return false;
        }
    }
}

bool Socket::read_len_prefixed_be(std::vector<uint8_t>& out, std::string& err)
{
    uint8_t hdr[4];
    if (!read_exact(hdr, 4, err)) {
        return false;
    }
    const uint32_t len = uint32_t(hdr[0]) << 24 | uint32_t(hdr[1]) << 16 | uint32_t(hdr[2]) << 8 | hdr[3];
    if (len < 4 || len > (32u << 20)) {
        err = MAA_IOS_TR("Invalid lockdown frame length: ") + std::to_string(len);
        return false;
    }
    out.resize(len);
    return read_exact(out.data(), len, err);
}

bool Socket::write_len_prefixed_be(std::string_view payload, std::string& err)
{
    uint8_t hdr[4];
    const uint32_t n = static_cast<uint32_t>(payload.size());
    hdr[0] = static_cast<uint8_t>(n >> 24);
    hdr[1] = static_cast<uint8_t>(n >> 16);
    hdr[2] = static_cast<uint8_t>(n >> 8);
    hdr[3] = static_cast<uint8_t>(n);
    if (!write_all(hdr, 4, err)) {
        return false;
    }
    return write_all(payload.data(), payload.size(), err);
}

} // namespace maa::ios::transport
