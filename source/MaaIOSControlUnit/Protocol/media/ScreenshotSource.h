#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "decode/Decoder.h"

namespace maa::ios::remote
{
class Device;
class ServiceConnection;
} // namespace maa::ios::remote

namespace maa::ios::media
{

/// 将截图服务返回的 PNG 解码为 BGRA。当前使用 FFmpeg；未包含该后端的构建返回
/// false 并说明原因。此函数不访问设备，可用 media_test 离线验证。
bool decode_png_bgra(const std::vector<uint8_t>& png, maa::ios::Frame& out, std::string& err);

/// 截图轮询画面源，用于显式截图模式或媒体流不可用时的降级。
///
/// 已测 iPadOS 18.7.8 的截图 RPC 约 0.45~0.5 秒，10 张含建连耗时 5.6 秒；刷新率受
/// 设备与连接耗时限制，不保证固定帧率。已测截图直接覆盖可见区，并按界面朝向摆正，
/// 不包含 HEVC 对齐填充，因此应用以零额外旋转显示，不依赖显示变化订阅。
class ScreenshotSource
{
public:
    /// 服务缺失、连接失败或同步首帧失败时返回 nullptr，并说明原因。
    ///
    /// capture_first=true 时，先同步取得并解码首张，再建立 worker，适用于首次启动。
    /// 运行中在渲染线程切换时传 false，首帧由 worker 异步获取，避免窗口等待网络请求。
    static std::unique_ptr<ScreenshotSource> start(remote::Device& device, std::string& err, bool capture_first = true);
    ~ScreenshotSource();

    ScreenshotSource(const ScreenshotSource&) = delete;
    ScreenshotSource& operator=(const ScreenshotSource&) = delete;

    /// 等待比 serial 更新的画面，成功时复制画面并更新序号。超时或停止时没有更新则返回 false。
    bool latest(maa::ios::Frame& out, uint64_t& serial, int timeout_ms);

    /// 请求 worker 停止并唤醒等待者，不等待线程退出。
    ///
    /// worker 可能仍在建连、截图 RPC、解码或失败退避中；本调用不取消这些步骤。
    /// 调用方暂存退役对象，之后按 worker_done() 通过 app::reap_finished 回收，析构时
    /// 才 join。截图源始终借用 Device，因此当前源和退役源都必须先于 Device 销毁。
    void request_stop();

    /// loop 已结束设备访问和成员更新时发布完成标记，不等待线程。
    /// 返回 true 后可析构并 join 剩余线程收尾。调用方逐帧回收已完成的源，
    /// 避免反复切换时把每张 BGRA 缓冲一直保留至整个会话退出。
    [[nodiscard]] bool worker_done() const { return worker_done_.load(std::memory_order_acquire); }

    struct Stats
    {
        uint64_t frames = 0;
        uint64_t bytes = 0;
        uint64_t failures = 0;
    };

    [[nodiscard]] Stats stats() const;
    /// 返回已解码画面尺寸；首帧尚未到达时为 0/0。
    void image_size(int& w, int& h) const;

private:
    explicit ScreenshotSource(remote::Device& device);
    bool capture_once(std::vector<uint8_t>& png, std::string& err);
    void loop();

    /// 已测截图服务复用连接后后续请求失败，因此每张截图建立独立连接。
    remote::Device& device_;
    std::thread worker_;
    std::atomic<bool> stopping_ { false };
    /// 与 stopping_ 区分：前者表示已提出停止请求，本标记表示 loop 已完成。
    /// 回收依据本标记，不能仅按 stopping_ 销毁仍在使用 Device 的对象。
    std::atomic<bool> worker_done_ { false };

    mutable std::mutex mu_;
    std::condition_variable cv_;
    maa::ios::Frame frame_;
    uint64_t serial_ = 0;
    Stats stats_;
};

} // namespace maa::ios::media
