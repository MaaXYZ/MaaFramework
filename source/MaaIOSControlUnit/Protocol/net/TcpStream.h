#pragma once
#include "net/ByteStream.h"
#include "net/Stack.h"
#include <memory>

namespace maa::ios::net
{
// 同步字节接口适配 lwIP raw TCP。协议状态只在核心线程访问；应用线程等待
// 独立的接收队列、连接结果与发送空间，不占用核心线程或隧道线程。
// Stack 必须比本端点活得更久；同一对象发起连接后不用于重新连接。
class TcpStream : public ByteStream
{
public:
    explicit TcpStream(Stack& stack);
    ~TcpStream() override;
    TcpStream(const TcpStream&) = delete;
    TcpStream& operator=(const TcpStream&) = delete;
    // 提交连接请求后在调用线程等待最多 15 秒；关闭或隧道失败会唤醒等待。
    // close 在 connect 前执行时也会永久关闭端点，不再创建 PCB。
    bool connect(uint16_t peer_port, std::string& err);
    // 并发发送按整个调用串行化，防止不同消息的字节交错；发送空间等待使用 15 秒截止时间。
    bool send(std::string_view data, std::string& err) override;
    // 返回当前已缓存的字节，不保留消息边界。设置 timed_out 时，超时单独报告且 err 留空。
    bool recv(std::vector<uint8_t>& out, int timeout_ms, std::string& err, bool* timed_out = nullptr) override;
    // 关闭不等待发送锁；核心线程撤销 PCB 并唤醒连接、发送及接收等待者。
    void close();
    bool connected() const;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace maa::ios::net
