#pragma once

#include <string>

#include "remote/Rsd.h"

namespace maa::ios::remote
{

class Device;

/// 设备上的应用启动、停止与列表接口。
///
/// `appservice` 使用 CoreDevice 的 feature / action 请求结构。启动请求中的
/// 应用标识和平台选项有特定的嵌套与类型要求：
///
/// ```text
/// { applicationSpecifier: { bundleIdentifier: { _0: "com.apple.mobilesafari" } },
///   options: { arguments: [], environmentVariables: {},
///              standardIOUsesPseudoterminals: true, startStopped: false,
///              terminateExisting: true, user: { shortName: "mobile" },
///              platformSpecificOptions: <Data: 序列化的 plist> },
///   standardIOIdentifiers: {} }
/// ```
///
/// 1. bundle ID 放在顶层 `applicationSpecifier.bundleIdentifier._0`。
///    `_0` 是 bundleIdentifier 枚举分支关联值的编码，不能将应用标识移入 options。
/// 2. `platformSpecificOptions` 使用 Data 携带可解析的 plist；没有额外平台选项时
///    发送空字典的 plist，而不是空字节或 XPC 字符串。
class App
{
public:
    /// 应用列表项。`path` 是设备上的安装目录，stop() 用它匹配进程的可执行文件路径。
    struct Entry
    {
        std::string bundle_id;
        std::string path;
        std::string name;
    };

    /// 启动应用。terminate_existing=true 请求终止已有实例后重新启动；false 允许
    /// 直接唤起已有实例。错误 10004 最多尝试四次，间隔 500 ms。
    static bool
        launch(Device& device, const std::string& bundle_id, std::string& err, bool terminate_existing = true, bool verbose = false);

    /// 按应用安装目录匹配进程，并逐个发送 SIGKILL。应用已安装但没有匹配进程时
    /// 返回成功。信号发送后尽力轮询退出状态；查询失败或轮询次数用尽时仍可能返回
    /// 成功并提示未确认退出，因此返回 true 不保证已观察到所有匹配进程消失。
    static bool stop(Device& device, const std::string& bundle_id, std::string& err, bool verbose = false);

    /// 列出设备上已安装的应用，包括系统应用。
    ///
    /// 使用 streamapplist 逐批接收，避免单条列表回复过大；设备未声明该功能时失败。
    /// out 逐批追加；传输重试前会清空，最终失败时可能保留已收到的部分结果。
    static bool list(Device& device, std::vector<Entry>& out, std::string& err, bool verbose = false);

    /// 构造 launchapplication 的 input，供启动调用及离线字段、嵌套结构检查复用。
    [[nodiscard]] static xpc::Value build_launch(const std::string& bundle_id, bool terminate_existing);

    /// 从 listprocesses 回复中选择可执行文件位于 app_path 目录下的进程号。
    /// 匹配包含目录分隔符边界，避免将 Foo.app2 误归入 Foo.app；可独立离线检查。
    static std::vector<int64_t> matching_pids(const xpc::Value& processes, const std::string& app_path);
};

} // namespace maa::ios::remote
