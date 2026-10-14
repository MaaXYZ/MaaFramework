#pragma once

#include <optional>
#include <stop_token>
#include <string>

#include "xpc/XpcValue.h"

namespace maa::ios::remote
{

class Device;

/// 设备剪贴板。
///
/// 写入或读回设备上的 UTF-8 文本。HID 键盘注入仅覆盖 US 布局的 ASCII，
/// 中文和 emoji 可以通过剪贴板传递。
///
/// pasteboardservice 直接接收带 command 字段的字典，不使用 CoreDevice invoke
/// 封装或 dtuhidd 的 messageType / payload / featureIdentifier 结构。
///
/// ```text
/// 写: {command:"SET",  pasteboardName:"general",
///      items:[{types:["public.utf8-plain-text"],
///              data:{"public.utf8-plain-text":{data:<裸字节>}}}]}   -> SET_REPLY
/// 读: {command:"PULL", pasteboardName:"general", dataPolicy:{allResolved:{}}}
///                                                                  -> PULL_REPLY
/// ```
///
/// 已观察到的连接与消息约束：
///
/// 1. 已测 dtpasteboardd 在同一连接收到第二个 WANTING_REPLY 请求时中止服务连接，
///    所以每次 set/get 使用独立连接。
/// 2. 已测设备在 types 为空时仍回复 SET_REPLY，但不会保存 data；必须同时声明类型。
///    回复命令正确只证明请求已回复，写入是否落地仍需读回或设备侧检查。
///
/// `data` 里的字节是原生 XPC Data，不是 base64。
class Pasteboard
{
public:
    /// 每次写入建立独立连接，包含零长度文本。cancel 中断本次连接和请求，不能
    /// 撤销设备已接受的 SET；reply_timeout_ms 只限制回复等待，不是操作总时限。
    static bool set_text(
        Device& device,
        const std::string& text,
        std::string& err,
        bool verbose = false,
        std::stop_token cancel = { },
        int reply_timeout_ms = 20000);
    /// 读回纯文本，零长度 Data 返回成功；缺少文本表示或请求失败时给出原因。
    static bool get_text(
        Device& device,
        std::string& out,
        std::string& err,
        bool verbose = false,
        std::stop_token cancel = { },
        int reply_timeout_ms = 20000);

    /// 构造包含 types 和原生 Data 的 SET 消息，供请求与离线形状检查共同使用。
    [[nodiscard]] static xpc::Value build_set(const std::string& text);
    /// 构造 PULL 的消息体。
    [[nodiscard]] static xpc::Value build_pull();
    /// 从 PULL_REPLY 里复制纯文本，包含空 Data；没有该表示形式时返回 nullopt。
    /// 返回值独立持有字节，不借用回信或共享缓存，可在下一次提取后继续使用。
    [[nodiscard]] static std::optional<std::string> find_text(const xpc::Value& reply);
};

} // namespace maa::ios::remote
