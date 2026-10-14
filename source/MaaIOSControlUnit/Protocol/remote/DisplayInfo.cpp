#include "remote/DisplayInfo.h"
#include "i18n/Translation.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>
#include <span>
#include <thread>
#include <utility>

#include "remote/Device.h"

namespace maa::ios::remote
{
namespace
{

constexpr std::string_view kService = "com.apple.coredevice.deviceinfo";
constexpr std::string_view kFeature = "com.apple.coredevice.feature.displayinfoupdates";

/// 接受 Int64、UInt64 和 Double 数值；显示几何推送可能用 Double 表示像素尺寸。
bool as_int(const xpc::Value* v, long long& out)
{
    if (v == nullptr) {
        return false;
    }
    switch (v->type) {
    case xpc::Type::Int64:
        out = v->int64;
        return true;
    case xpc::Type::UInt64:
        if (v->uint64 > static_cast<uint64_t>(std::numeric_limits<long long>::max())) {
            return false;
        }
        out = static_cast<long long>(v->uint64);
        return true;
    case xpc::Type::Double: {
        if (!std::isfinite(v->real)) {
            return false;
        }
        // Double 按最近整数转换，避免直接截断几何值的小数部分。
        const double rounded = std::round(v->real);
        // LLONG_MAX 转为 Double 会向上取到 2^63，不能拿它作闭区间上界。
        // 使用精确的 2^digits 排他上界，先检查再转换，避免浮点转整数越界。
        const double upper = std::ldexp(1.0, std::numeric_limits<long long>::digits);
        if (rounded < static_cast<double>(std::numeric_limits<long long>::min()) || rounded >= upper) {
            return false;
        }
        out = static_cast<long long>(rounded);
        return true;
    }
    default:
        return false;
    }
}

bool as_bool(const xpc::Value* v, bool& out)
{
    if (v == nullptr || v->type != xpc::Type::Bool) {
        return false;
    }
    out = v->boolean;
    return true;
}

std::string as_string(const xpc::Value* v)
{
    return v != nullptr && v->type == xpc::Type::String ? v->string : std::string();
}

/// 读取数组的前两个数值作为 [w, h]，支持整数和 Double。
bool as_pair(const xpc::Value* v, int& w, int& h)
{
    if (v == nullptr || v->type != xpc::Type::Array || v->array.size() < 2) {
        return false;
    }
    long long a = 0, b = 0;
    if (!as_int(&v->array[0], a) || !as_int(&v->array[1], b)) {
        return false;
    }
    if (a < std::numeric_limits<int>::min() || a > std::numeric_limits<int>::max() || b < std::numeric_limits<int>::min()
        || b > std::numeric_limits<int>::max()) {
        return false;
    }
    w = static_cast<int>(a);
    h = static_cast<int>(b);
    return true;
}

/// 包装 actualInput 和 streamProxy.sideChannel；每次订阅生成新的 16 字节 UUID。
[[nodiscard]] xpc::Value stream_input()
{
    std::vector<uint8_t> side(16);
    for (auto& b : side) {
        b = static_cast<uint8_t>(std::random_device { }());
    }
    auto proxy = xpc::make_dict();
    xpc::dict_set(proxy, "sideChannel", xpc::make_uuid(std::span<const uint8_t>(side)));
    auto input = xpc::make_dict();
    xpc::dict_set(input, "actualInput", xpc::make_dict());
    xpc::dict_set(input, "streamProxy", std::move(proxy));
    return input;
}

} // namespace

const Display* DisplayInfo::find(uint64_t id) const
{
    for (const auto& d : displays) {
        if (d.id == id) {
            return &d;
        }
    }
    return nullptr;
}

const Display* DisplayInfo::primary() const
{
    for (const auto& d : displays) {
        if (d.primary) {
            return &d;
        }
    }
    return nullptr;
}

std::optional<DisplayInfo> parse_display_info(const xpc::Value& element, std::string& err)
{
    const auto* list = element.find("displays");
    if (list == nullptr || list->type != xpc::Type::Array) {
        err = MAA_IOS_TR("Update missing displays array: ") + xpc::describe(element).substr(0, 200);
        return std::nullopt;
    }
    DisplayInfo info;
    for (const auto& raw : list->array) {
        const auto* id = raw.find("displayId");
        if (id == nullptr) {
            continue; // 无 displayId 的条目无法选择，跳过该条目。
        }
        long long raw_id = 0;
        if (!as_int(id, raw_id) || raw_id < 0) {
            continue;
        }
        Display d;
        d.id = static_cast<uint64_t>(raw_id);
        as_bool(raw.find("primary"), d.primary);
        as_bool(raw.find("external"), d.external);
        d.name = as_string(raw.find("name"));
        if (d.name.empty()) {
            // name 缺失时使用 deviceName。
            d.name = as_string(raw.find("deviceName"));
        }
        // 当前模式尺寸取自 currentMode.size，缺失时保留默认尺寸供使用方判断。
        const auto* mode = raw.find("currentMode");
        int w = 0, h = 0;
        if (mode != nullptr && as_pair(mode->find("size"), w, h)) {
            d.width = w;
            d.height = h;
        }
        d.orientation = as_string(raw.find("currentOrientation"));
        info.displays.push_back(std::move(d));
    }
    if (const auto* o = element.find("orientation"); o != nullptr) {
        info.device_orientation = as_string(o->find("currentDeviceOrientation"));
    }
    if (info.displays.empty()) {
        err = MAA_IOS_TR("No recognized display entries");
        return std::nullopt;
    }
    return info;
}

std::optional<DisplayInfo> fetch_display_info(Device& device, std::string& err, bool verbose)
{
    auto conn = device.connect(kService, err, verbose);
    if (conn == nullptr) {
        return std::nullopt;
    }
    const auto input = stream_input();

    std::optional<DisplayInfo> out;
    std::string parse_err;
    const auto r = conn->stream(
        kFeature,
        "",
        input,
        [&](const xpc::Value& element) {
            out = parse_display_info(element, parse_err);
            return out == std::nullopt; // 取得可解析推送后结束本地消费。
        },
        6000,
        err);
    if (out != std::nullopt) {
        return out;
    }
    if (r == CallResult::Ok && parse_err.empty()) {
        err = std::string(kFeature) + MAA_IOS_TR(" ended without an update");
    }
    else if (!parse_err.empty()) {
        err = parse_err;
    }
    return std::nullopt;
}

// 常驻显示订阅。

namespace
{

/// 单次 next_batch 等待上限；重连还需建立连接并发送订阅，因此不是整体退出时限。
constexpr int kPollMs = 250;
/// 订阅结束或失败后的重连间隔，等待期间分段检查停止标志。
constexpr int kReconnectGapMs = 500;
/// 连续重订失败时的停止阈值，避免失去设备后无限重连。
constexpr int kMaxFailedResubscribes = 8;

} // namespace

DisplayWatcher::DisplayWatcher(Device& device, uint64_t display_id, bool verbose)
    : device_(device)
    , display_id_(display_id)
    , verbose_(verbose)
{
}

std::unique_ptr<DisplayWatcher> DisplayWatcher::start(Device& device, uint64_t display_id, std::string& err, bool verbose)
{
    auto watcher = std::unique_ptr<DisplayWatcher>(new DisplayWatcher(device, display_id, verbose));
    // 首次连接和订阅完成后才启动 worker；此时连接尚未由其它线程访问。
    if (!watcher->resubscribe(err)) {
        return nullptr;
    }
    watcher->alive_.store(true);
    watcher->worker_ = std::thread(&DisplayWatcher::loop, watcher.get());
    return watcher;
}

DisplayWatcher::~DisplayWatcher()
{
    // 仅设置停止标志，等待 worker 完成当前收消息或重连操作后，再析构连接。
    stop_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
}

DisplayWatcher::State DisplayWatcher::latest() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return latest_;
}

const Display* DisplayWatcher::pick(const DisplayInfo& info) const
{
    const Display* d = info.find(display_id_);
    if (d == nullptr) {
        d = info.primary(); // 请求 id 未出现在当前推送时退回主屏。
    }
    // 目录中可能有尚无有效模式的显示屏，非正尺寸不能用于布局。
    if (d != nullptr && (d->width <= 0 || d->height <= 0)) {
        return nullptr;
    }
    return d;
}

bool DisplayWatcher::resubscribe(std::string& err)
{
    conn_ = device_.connect(kService, err, verbose_);
    if (conn_ == nullptr) {
        return false;
    }
    if (!conn_->subscribe(kFeature, "", stream_input(), err)) {
        conn_.reset();
        return false;
    }
    return true;
}

void DisplayWatcher::publish(const xpc::Value& element)
{
    std::string err;
    const auto info = parse_display_info(element, err);
    if (info == std::nullopt) {
        return; // 不可解析的推送不替换已有快照。
    }
    const Display* d = pick(*info);
    if (d == nullptr) {
        return;
    }
    // 网络读取和解析均在锁外完成，仅在此比较和发布完整几何快照。
    std::lock_guard<std::mutex> lock(mu_);
    if (d->orientation == latest_.orientation && d->width == latest_.width && d->height == latest_.height) {
        return; // 重复几何不产生新的 seq。
    }
    latest_.orientation = d->orientation;
    latest_.width = d->width;
    latest_.height = d->height;
    ++latest_.seq;
}

void DisplayWatcher::loop()
{
    std::vector<xpc::Value> batch;
    int failures = 0;
    while (!stop_.load()) {
        std::string err;
        // 重订失败会留下空连接；沿断流分支继续退避重订，不读取不存在的连接。
        const auto event = conn_ != nullptr ? conn_->next_batch(batch, kPollMs, err) : ServiceConnection::StreamEvent::Broken;
        if (event == ServiceConnection::StreamEvent::Batch) {
            failures = 0;
            for (const auto& element : batch) {
                publish(element);
            }
            continue;
        }
        if (event == ServiceConnection::StreamEvent::Idle) {
            continue; // 本轮没有推送，保留订阅和最后一次有效状态。
        }
        // Finished、DeviceError 或 Broken 后释放旧连接，按重连策略重新订阅。
        conn_.reset();
        // 将退避拆成 20 ms 片段，停止请求无需等完整个重连间隔。
        for (int slept = 0; slept < kReconnectGapMs && !stop_.load(); slept += 20) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if (stop_.load()) {
            break;
        }
        if (resubscribe(err)) {
            failures = 0;
            continue;
        }
        if (++failures >= kMaxFailedResubscribes) {
            // 达到停止阈值后清除运行标记，保留已发布的显示快照。
            alive_.store(false);
            return;
        }
    }
    alive_.store(false);
}

} // namespace maa::ios::remote
