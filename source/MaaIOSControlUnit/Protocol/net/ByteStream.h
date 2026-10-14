#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::net
{

// RemoteXPC 只依赖已建立连接的字节传输。连接建立和协议栈生命周期由调用方管理，
// 因而可以用同一份 RemoteXPC 实现验证不同 TCP 后端。
class ByteStream
{
public:
    virtual ~ByteStream() = default;
    // 成功表示全部字节已交给传输后端，不表示对端应用已读取或确认。
    virtual bool send(std::string_view data, std::string& err) = 0;
    // 成功时返回非空数据；给出 timed_out 时，超时返回 false、清空 err 并置 true。
    // 未给出 timed_out 时，超时也通过 err 报告。
    // 对端关闭或传输失败时返回 false，err 包含原因，timed_out 为 false。
    virtual bool recv(std::vector<uint8_t>& out, int timeout_ms, std::string& err, bool* timed_out = nullptr) = 0;
};

} // namespace maa::ios::net
