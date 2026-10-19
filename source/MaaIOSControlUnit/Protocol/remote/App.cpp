#include "remote/App.h"

#include "MaaUtils/Logger.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

#include "plist/Plist.h"
#include "remote/Device.h"

namespace maa::ios::remote
{
namespace
{

constexpr std::string_view kService = "com.apple.coredevice.appservice";
constexpr std::string_view kFeature = "com.apple.coredevice.feature.launchapplication";
constexpr std::string_view kAction = "com.apple.coredevice.action.launch";
constexpr int kSigKill = 9;

/// 将空字典序列化为 plist 字节，作为 platformSpecificOptions 的 Data。
/// 没有平台选项时发送空字典的 plist，不能使用零长度 Data。
std::vector<uint8_t> empty_plist()
{
    const std::string text = plist::write(plist::Value::Dict());
    return { text.begin(), text.end() };
}

/// 对可重复的查询 RPC，仅在 TransportError 时重试，最多三次。
/// feature_call 每次创建新服务连接；设备明确返回 DeviceError 时直接交给调用方。
CallResult ask_idempotent(
    Device& device,
    std::string_view feature_identifier,
    const xpc::Value& input,
    xpc::Value& output,
    std::string& err,
    bool verbose,
    int timeout_ms)
{
    for (int attempt = 1; attempt <= 3; ++attempt) {
        const auto result = device.feature_call(kService, feature_identifier, "", input, output, err, verbose, timeout_ms);
        if (result != CallResult::TransportError || attempt == 3) {
            return result;
        }
    }
    return CallResult::TransportError;
}

/// 移除 executableURL 的 file:// 前缀，以便与应用列表中不带 scheme 的路径比较。
/// 这里只移除前缀，不做 URL 解码或其他路径规范化。
std::string strip_file_scheme(std::string_view url)
{
    constexpr std::string_view kPrefix = "file://";
    if (url.starts_with(kPrefix)) {
        url.remove_prefix(kPrefix.size());
    }
    return std::string(url);
}

} // namespace

xpc::Value App::build_launch(const std::string& bundle_id, bool terminate_existing)
{
    // applicationSpecifier 使用 bundleIdentifier 枚举分支，关联的 bundle ID 字符串
    // 编码在 _0 下。specifier 与 options 是顶层并列字段。
    auto which = xpc::make_dict();
    xpc::dict_set(which, "_0", xpc::make_string(bundle_id));
    auto specifier = xpc::make_dict();
    xpc::dict_set(specifier, "bundleIdentifier", std::move(which));

    auto options = xpc::make_dict();
    xpc::dict_set(options, "arguments", xpc::make_array());
    xpc::dict_set(options, "environmentVariables", xpc::make_dict());
    xpc::dict_set(options, "standardIOUsesPseudoterminals", xpc::make_bool(true));
    xpc::dict_set(options, "startStopped", xpc::make_bool(false));
    xpc::dict_set(options, "terminateExisting", xpc::make_bool(terminate_existing));
    auto user = xpc::make_dict();
    xpc::dict_set(user, "shortName", xpc::make_string("mobile"));
    xpc::dict_set(options, "user", std::move(user));
    xpc::dict_set(options, "platformSpecificOptions", xpc::make_data(empty_plist()));

    auto input = xpc::make_dict();
    xpc::dict_set(input, "applicationSpecifier", std::move(specifier));
    xpc::dict_set(input, "options", std::move(options));
    xpc::dict_set(input, "standardIOIdentifiers", xpc::make_dict());
    return input;
}

bool App::launch(Device& device, const std::string& bundle_id, std::string& err, bool terminate_existing, bool verbose)
{
    const auto input = build_launch(bundle_id, terminate_existing);
    // 错误 10004 表示未取得新进程标识，允许有限重试。
    // feature() 将设备错误汇总到 err 字符串，因此以错误码文本判断。
    constexpr int kAttempts = 4;
    for (int attempt = 1; attempt <= kAttempts; ++attempt) {
        xpc::Value output;
        // 单次启动 RPC 使用 60 秒超时，包含按选项终止旧实例、启动及获取进程标识。
        if (device.feature(kService, kFeature, kAction, input, output, err, verbose, 60000)) {
            return true;
        }
        if (err.find("10004") == std::string::npos || attempt == kAttempts) {
            return false;
        }
        LogWarn << std::format("Launching {}: previous process has not exited (attempt {}); retry in 500 ms", bundle_id, attempt);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return false;
}

std::vector<int64_t> App::matching_pids(const xpc::Value& processes, const std::string& app_path)
{
    std::vector<int64_t> out;
    const auto* tokens = processes.find("processTokens");
    if (tokens == nullptr || app_path.empty()) {
        return out;
    }
    for (const auto& token : tokens->array) {
        const std::string exe = strip_file_scheme(token.at("executableURL").at("relative").as_string_or(""));
        // 将 '/' 纳入前缀匹配，限定安装目录边界，避免 Foo.app 同时匹配 Foo.app2。
        if (exe.starts_with(app_path + "/")) {
            out.push_back(token.at("processIdentifier").as_int_or(0));
        }
    }
    return out;
}

bool App::list(Device& device, std::vector<Entry>& out, std::string& err, bool verbose)
{
    if (!device.rsd().supports(kService, "com.apple.coredevice.feature.streamapplist")) {
        err = "Device appservice does not advertise streamapplist; large listapps fallback is disabled";
        return false;
    }

    auto flags = xpc::make_dict();
    for (const char* k : { "includeAppClips",
                           "includeRemovableApps",
                           "includeHiddenApps",
                           "includeInternalApps",
                           "includeDefaultApps",
                           "includeContainerPaths",
                           "includeAppGroupIdentifiers" }) {
        xpc::dict_set(flags, k, xpc::make_bool(true));
    }
    // 本次列表请求不要求容器访问授权。
    xpc::dict_set(flags, "requireContainerAccess", xpc::make_bool(false));

    // 用随机的 16 字节值作为 sideChannel 的 XPC UUID。当前连接只承载这一条列表流，
    // 本实现没有额外校验回复中的 sideChannel 标识。
    std::vector<uint8_t> side(16);
    for (auto& b : side) {
        b = static_cast<uint8_t>(std::random_device { }());
    }
    auto proxy = xpc::make_dict();
    xpc::dict_set(proxy, "sideChannel", xpc::make_uuid(side));
    auto input = xpc::make_dict();
    xpc::dict_set(input, "actualInput", std::move(flags));
    xpc::dict_set(input, "streamProxy", std::move(proxy));

    // 仅对已连接后的流式调用 TransportError 重试，重新连接失败则直接返回。
    const auto collect = [&out](const xpc::Value& one) {
        Entry e {
            .bundle_id = one.at("bundleIdentifier").as_string_or(""),
            .path = one.at("path").as_string_or(""),
            .name = one.at("name").as_string_or(""),
        };
        if (!e.bundle_id.empty()) {
            out.push_back(std::move(e));
        }
        return true;
    };
    for (int attempt = 1; attempt <= 3; ++attempt) {
        auto conn = device.connect(kService, err, verbose);
        if (conn == nullptr) {
            return false;
        }
        const auto got = conn->stream("com.apple.coredevice.feature.streamapplist", "", input, collect, 30000, err);
        if (got != CallResult::TransportError || attempt == 3) {
            return got == CallResult::Ok;
        }
        out.clear(); // 舍弃上一轮的部分结果，避免重新拉取时重复追加。
    }
    return false;
}

bool App::stop(Device& device, const std::string& bundle_id, std::string& err, bool verbose)
{
    std::vector<Entry> apps;
    if (!list(device, apps, err, verbose)) {
        return false;
    }
    const auto it = std::ranges::find(apps, bundle_id, &Entry::bundle_id);
    if (it == apps.end()) {
        err = "Device has no app with bundle ID " + bundle_id;
        return false;
    }

    xpc::Value procs;
    if (ask_idempotent(device, "com.apple.coredevice.feature.listprocesses", xpc::make_dict(), procs, err, verbose, 30000)
        != CallResult::Ok) {
        return false;
    }
    const auto pids = matching_pids(procs, it->path);
    if (pids.empty()) {
        // 应用已安装，但当前进程表没有匹配项，无需发送信号即可返回成功。
        LogDebug << std::format("stop_app({}): app is not running", bundle_id);
        return true;
    }
    for (const int64_t pid : pids) {
        auto input = xpc::make_dict();
        auto process = xpc::make_dict();
        xpc::dict_set(process, "processIdentifier", xpc::make_int64(pid));
        xpc::dict_set(input, "process", std::move(process));
        xpc::dict_set(input, "signal", xpc::make_int64(kSigKill));
        // 传输失败不能确认设备是否已执行信号请求；当前对这类失败最多尝试三次，
        // 每次使用新服务连接。设备明确拒绝时立即失败，不继续重试。
        bool killed = false;
        for (int attempt = 1; attempt <= 3 && !killed; ++attempt) {
            xpc::Value output;
            const auto result =
                device.feature_call(kService, "com.apple.coredevice.feature.sendsignaltoprocess", "", input, output, err, verbose, 30000);
            if (result == CallResult::Ok) {
                killed = true;
            }
            else if (result == CallResult::DeviceError) {
                err = "Failed to terminate " + bundle_id + " process " + std::to_string(pid) + ": " + err;
                return false;
            }
        }
        if (!killed) {
            err = "Failed to terminate " + bundle_id + " process " + std::to_string(pid) + "; no reply after three attempts: " + err;
            return false;
        }
    }

    // 信号请求成功不等于进程已退出。轮询进程表，尽量避免紧接着启动时遇到状态竞态。
    // 最多查询 40 次，查询之间等待 250 ms；每次 RPC 仍有各自的超时和传输重试。
    // 查询失败或次数用尽时，下面仍按信号已发送返回成功，并提示未确认退出。
    for (int wait = 0; wait < 40; ++wait) {
        xpc::Value again;
        std::string perr;
        if (ask_idempotent(device, "com.apple.coredevice.feature.listprocesses", xpc::make_dict(), again, perr, verbose, 30000)
            != CallResult::Ok) {
            break; // 查询失败后停止等待；此前已成功发送信号请求。
        }
        if (matching_pids(again, it->path).empty()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    LogWarn << std::format("stop_app({}): signal sent, but process is still listed (exit may be pending)", bundle_id);
    return true;
}

} // namespace maa::ios::remote
