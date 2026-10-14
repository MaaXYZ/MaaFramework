#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "remote/Rsd.h"
#include "xpc/XpcValue.h"

namespace maa::ios::remote
{

class Device;

/// displayinfoupdates 推送中的显示状态，用于可见区尺寸、旋转和输入坐标映射。
/// 当前模式的尺寸来自设备显示服务；媒体帧的编码尺寸可能包含对齐填充。
/// 这里只解析应用使用的 displayId、主屏/外接标记、名字、当前模式尺寸及朝向。
struct Display
{
    /// 设备分配的 displayId，与 startmediastream 请求选择的显示屏对应。
    uint64_t id = 0;
    bool primary = false;
    bool external = false;
    /// name 字段，缺失时使用 deviceName，供日志显示。
    std::string name;
    /// currentMode.size 报告的当前模式像素尺寸；不读取面板 nativeSize。
    /// 字段缺失或不能转换时保留 0，由使用方判断尺寸是否可用。
    int width = 0;
    int height = 0;
    /// `currentOrientation`：`rot0` / `rot90` / `rot180` / `rot270`。
    std::string orientation;
};

struct DisplayInfo
{
    std::vector<Display> displays;
    /// orientation.currentDeviceOrientation，供诊断设备朝向；显示旋转使用各屏 orientation。
    std::string device_orientation;

    /// 按 displayId 查询；未找到返回 nullptr，返回指针借用本快照中的条目。
    [[nodiscard]] const Display* find(uint64_t id) const;
    /// 返回首个 primary=true 的显示屏；没有主屏条目时返回 nullptr。
    [[nodiscard]] const Display* primary() const;
};

/// 从一条 element 解析显示状态。displays 必须为数组，无可识别条目时返回 nullopt。
/// 无有效 displayId 的条目被跳过，缺少尺寸/朝向的条目仍可保留，交由使用方筛选。
[[nodiscard]] std::optional<DisplayInfo> parse_display_info(const xpc::Value& element, std::string& err);

/// 建立临时 deviceinfo 订阅，取得第一条可解析推送后释放连接。
/// 每批等待上限为 6 秒；解析失败可继续等待，因此并非整个操作的总时限。
std::optional<DisplayInfo> fetch_display_info(Device& device, std::string& err, bool verbose = false);

/// 常驻显示订阅：独占一条服务连接，由 worker 读取推送并在必要时重订。
/// latest() 在互斥锁内复制尺寸、朝向和序号，调用方取得的是同一次发布的快照。
/// 持有 Device 引用，必须先于 Device 析构。析构请求停止并 join，再释放连接。
/// 单次收消息使用短轮询，连接建立及订阅发送仍可能有各自的等待，退出没有统一短时限。
class DisplayWatcher
{
public:
    /// 最近一次有效推送中选定显示屏的几何；第一条有效推送前保持默认值。
    struct State
    {
        int width = 0;
        int height = 0;
        /// `rot0` / `rot90` / `rot180` / `rot270`。
        std::string orientation;
        /// 尺寸或朝向发生变化时递增；重复推送不改变序号，调用方可据此更新布局。
        uint64_t seq = 0;
    };

    /// 在调用线程完成首次订阅，再启动 worker；首次失败返回 nullptr 并填写 err。
    static std::unique_ptr<DisplayWatcher> start(Device& device, uint64_t display_id, std::string& err, bool verbose = false);

    ~DisplayWatcher();

    DisplayWatcher(const DisplayWatcher&) = delete;
    DisplayWatcher& operator=(const DisplayWatcher&) = delete;

    [[nodiscard]] State latest() const;

    /// 工作线程的运行标记；正常结束时置 false，不清除最后一次有效快照。
    [[nodiscard]] bool alive() const { return alive_.load(); }

private:
    DisplayWatcher(Device& device, uint64_t display_id, bool verbose);

    void loop();
    bool resubscribe(std::string& err);
    void publish(const xpc::Value& element);
    /// 先按请求 id 选择，缺失时退回主屏；非正尺寸的条目不可发布。
    const Display* pick(const DisplayInfo& info) const;

    Device& device_;
    uint64_t display_id_;
    bool verbose_;
    /// 首次订阅在创建 worker 前完成；之后仅由 worker 读取和替换连接。
    std::unique_ptr<ServiceConnection> conn_;
    std::thread worker_;
    /// 仅保护 latest_ 的发布和复制，不跨网络调用持锁。
    mutable std::mutex mu_;
    State latest_;
    std::atomic<bool> stop_ { false };
    std::atomic<bool> alive_ { false };
};

} // namespace maa::ios::remote
