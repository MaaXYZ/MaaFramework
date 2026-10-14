#pragma once

#include "transport/SocketPlatform.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::transport
{

/// 持有一个本机 socket，移动时转移所有权。
///
/// Socket 拥有句柄并负责关闭。支持 SO_NOSIGPIPE 的平台在接管时设置该选项，
/// 因为同一条连接随后可能交给 OpenSSL，其 socket BIO 不经过本类的 send。
class Socket
{
public:
    Socket() = default;
    explicit Socket(NativeSocket fd);
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    ~Socket();

    [[nodiscard]] NativeSocket fd() const { return fd_; }

    [[nodiscard]] bool valid() const
    {
#ifdef _WIN32
        return fd_ != kInvalidSocket;
#else
        return fd_ >= 0;
#endif
    }

    [[nodiscard]] NativeSocket release()
    {
        NativeSocket fd = fd_;
        fd_ = kInvalidSocket;
        return fd;
    }

    void reset(NativeSocket fd);
    void close();
    // 保留句柄所有权，中断其他线程正在执行的 I/O；线程退出后再关闭。
    void interrupt();

    bool write_all(const void* data, size_t len, std::string& err);
    bool read_exact(void* data, size_t len, std::string& err);

    /// 等待可读，最多 ms 毫秒。返回 false 表示超时或出错。
    /// 给出 timed_out 时，超时会置 true 并清空 err；系统错误仍返回原因。
    bool wait_readable(int ms, std::string& err, bool* timed_out = nullptr);

    /// 设置单次 recv 的 SO_RCVTIMEO，ms 为 0 时不设超时。
    /// 避免对端未发送 FIN 时永久等待；多次读取的总期限仍由调用方控制。
    bool set_read_timeout(int ms, std::string& err);

    /// 读一个 lockdown 风格的帧：4 字节**大端**长度 + 该长度的负载。
    bool read_len_prefixed_be(std::vector<uint8_t>& out, std::string& err);
    bool write_len_prefixed_be(std::string_view payload, std::string& err);

private:
    NativeSocket fd_ = kInvalidSocket;
};

} // namespace maa::ios::transport
