#pragma once

#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>

#include "remote/Rsd.h"

namespace maa::ios::remote
{

class Device;

enum class DeviceOrientation
{
    Unknown,
    Portrait,
    PortraitUpsideDown,
    LandscapeLeft,
    LandscapeRight,
    FaceUp,
    FaceDown
};

struct OrientationState
{
    DeviceOrientation current = DeviceOrientation::Unknown;
    DeviceOrientation nonflat = DeviceOrientation::Unknown;
    /// 服务报告的状态；尚不能据此认定是控制中心的用户竖排方向锁。
    bool locked = false;
};

/// CoreDevice devicecontrol 的方向控制会话。调用方串行使用并先于 Device 销毁。
/// 请求使用 OrientationRequest 直接外壳，不使用 CoreDevice invoke。
/// 每次 RPC 独占新服务连接：已测设备在同一连接的第二次 RPC 执行后断链而不回信。
/// 没有已验证的解锁/原策略恢复接口，析构仅关闭预建连接，不发送 restore。
class OrientationControl
{
public:
    static constexpr std::string_view kService = "com.apple.coredevice.devicecontrol";
    static constexpr std::string_view kFeature = "com.apple.coredevice.feature.remote.devicecontrol.orientation";

    /// 验证能力并预建首个 RPC 的连接，后续每个 RPC 独立建连；cancel 覆盖全部连接。
    /// 停止后只能关闭传输，不能撤销已发请求。Device 须覆盖本对象的整个生命期。
    static std::unique_ptr<OrientationControl>
        connect(Device& device, std::string& err, bool verbose = false, std::stop_token cancel = { });
    ~OrientationControl() = default;
    OrientationControl(const OrientationControl&) = delete;
    OrientationControl& operator=(const OrientationControl&) = delete;

    /// timeout_ms 仅限这次回复等待，不含新建服务连接，须为正数；失败时不改写 out。
    CallResult query(OrientationState& out, std::string& err, int timeout_ms = 3000);
    /// 只接受四个 cardinal 值，发送恰好一次，无自动重试、解锁或恢复。
    /// Ok 表示服务返回有效状态，不保证应用 UI 旋转或用户锁策略不变。
    /// TransportError 表示执行状态未知，调用方不得据此盲目重发。
    CallResult change(DeviceOrientation target, OrientationState& out, std::string& err, int timeout_ms = 3000);

    [[nodiscard]] static xpc::Value build_query();
    [[nodiscard]] static std::optional<xpc::Value> build_change(DeviceOrientation target);
    /// 仅接受已观察到的三个直接字段，无臆测 success/error 外壳或数值转换。
    [[nodiscard]] static std::optional<OrientationState> parse_state(const xpc::Value& reply);
    [[nodiscard]] static std::string_view name(DeviceOrientation orientation);
    [[nodiscard]] static bool cardinal(DeviceOrientation orientation);
    /// 手机平放时 current 可能为 faceUp/faceDown；只用有效 nonflat 作为基线。
    /// 两者都无 cardinal 时返回 nullopt，不猜 portrait。
    [[nodiscard]] static std::optional<DeviceOrientation> effective_cardinal(const OrientationState& state);

private:
    OrientationControl(Device& device, std::unique_ptr<ServiceConnection> connection, bool verbose, std::stop_token cancel)
        : device_(device)
        , connection_(std::move(connection))
        , verbose_(verbose)
        , cancel_(cancel)
    {
    }

    CallResult exchange(const xpc::Value& request, OrientationState& out, std::string& err, int timeout_ms);
    Device& device_;
    /// 仅 connect() 预建首个连接。exchange() 取走它，任何 RPC 结束后都不保留连接。
    std::unique_ptr<ServiceConnection> connection_;
    bool verbose_;
    std::stop_token cancel_;
};

} // namespace maa::ios::remote
