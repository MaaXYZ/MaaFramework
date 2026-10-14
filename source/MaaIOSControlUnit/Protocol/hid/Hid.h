#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "remote/Device.h"
#include "xpc/XpcValue.h"

namespace maa::ios::hid
{

/// 触摸和键盘通过 `com.apple.coredevice.hid.universalhidservice` 注入。
/// 设备在 dtuhidd 注册 HID 面，宿主按目标 `_ServiceID` 发送原始报告。
/// 触摸坐标使用归一化的 0..65535，报告构造无需知道屏幕像素尺寸；坐标轴与
/// 屏幕方向的对应关系仍需由调用方处理。
///
/// 已有设备验证表明：媒体会话运行、结束以及从未启动时，触摸报告都能生效。
/// 因此本模块不以运行中的媒体会话为输入前提，也不为媒体状态增加等待。
/// 该结论限于已有设备和 DDI 配置；认证状态的完整机制尚未确定。
/// 验证记录见 docs/coredevice.md 第 12 节。
///
/// `hid.indigo` 的 digitizer、keyboard 和 scroll 路径使用 Apple Mercury 事件包装，
/// 当前尚未适配，已有请求未能完成注入。本模块的触摸与键盘使用 universalhidservice；
/// 硬件按键使用 indigo 的 button。

/// 已测设备注册的 HID 面标识；可用面仍应以设备返回的 connectedServices 为准。
inline constexpr uint64_t kSurfaceMainTouchscreen = 257; ///< 主触摸屏（0x101）
inline constexpr uint64_t kSurfaceKeyboard = 512;        ///< 设备自带的虚拟键盘（0x200）
inline constexpr uint64_t kSurfaceTouchGesture = 1281;   ///< 触控板式指针（0x501）

/// mainTouchscreen 报告里的接触状态字节。
inline constexpr uint8_t kStateContact = 0xC2; ///< 在该位置保持接触
inline constexpr uint8_t kStateRelease = 0x02; ///< 抬起

/// 以本进程首次调用为起点，读取 steady_clock 的纳秒差值，返回低 48 位。
/// 时间戳用于表达报告间隔，手势速度依赖这些差值，不需要与设备墙钟对齐。
/// 低 48 位在范围耗尽后会回绕；未回绕时保留单调时钟的顺序和实际间隔。
[[nodiscard]] uint64_t report_timestamp();

/// 58 字节的 mainTouchscreen 报告（报告号 0x09）。
///
/// ```text
///  0     0x09  报告号
///  1-2   0x01 0x05
///  3     状态：0xC2 接触 / 0x02 抬起
///  4-5   X  (UInt16 LE, 0..65535)
///  6-7   Y  (UInt16 LE, 0..65535)
///  8-39  32 字节 0
///  40-43 0x02 0x00 0x00 0x00
///  44-49 时间戳（6 字节 LE）
///  50-57 8 字节 0
/// ```
///
/// 点击在同一坐标依次发送 CONTACT 和 RELEASE；拖动逐点发送 CONTACT，最后
/// 发送 RELEASE。CONTACT 表示该位置当前保持接触，报告没有独立的 begin/end 操作码。
/// 坐标和时间戳按小端写入，timestamp 只写低 48 位，其余保留字节保持为零。
[[nodiscard]] std::vector<uint8_t> touchscreen_report(uint8_t state, uint16_t x, uint16_t y, uint64_t timestamp = report_timestamp());

/// 将 0.0..1.0 坐标映射到 UInt16，区间内按最近整数取值。
/// 小于等于 0 或 NaN 返回 0，大于等于 1 返回 65535；越界值不取模。
[[nodiscard]] uint16_t normalize(double v);

/// 按 US 键盘布局把可打印 ASCII、制表符和换行符转换为报告序列。
/// 每个条目都是当前按住的完整 usage 集合：普通字符为 {键}、{}；需要 Shift 的
/// 字符为 {ShiftLeft}、{ShiftLeft, 键}、{}，最后的空集合释放全部按键。
/// 输入按字节遍历，非 ASCII 或其它未支持字符会被跳过，不返回错误。
/// 此函数不解码 Unicode，也不根据设备当前键盘布局调整映射。
[[nodiscard]] std::vector<std::vector<uint16_t>> text_reports(const std::string& text);

/// USB-IF HID Usage Page 0x07 中用于 ASCII 映射和组合键的键码。
namespace key
{
inline constexpr uint16_t kA = 0x04; ///< a..z 连续排到 0x1D
inline constexpr uint16_t kZ = 0x1D;
inline constexpr uint16_t k1 = 0x1E; ///< 1..9 到 0x26，0 是 0x27
inline constexpr uint16_t k0 = 0x27;
inline constexpr uint16_t kEnter = 0x28;
inline constexpr uint16_t kEsc = 0x29;
inline constexpr uint16_t kBackspace = 0x2A;
inline constexpr uint16_t kTab = 0x2B;
inline constexpr uint16_t kSpace = 0x2C;
inline constexpr uint16_t kShiftLeft = 0xE1; ///< 0xE0..0xE7 为左右修饰键
inline constexpr uint16_t kGuiLeft = 0xE3;   ///< 即 iOS 上的 Command 键
} // namespace key

/// 39 字节的虚拟键盘报告（报告号 0x01）。
///
/// ```text
///  0     0x01  报告号
///  1-30  240 位 usage 位图（LE 位序）：usage u 按下 <=> 字节 1 + u/8 的第 u%8 位
///  31-36 时间戳（6 字节 LE）
///  37-38 保留
/// ```
///
/// 报告携带当前按住的完整集合，释放按键时发送移除该 usage 后的集合；
/// 空集合释放全部按键，没有独立的 release 操作码。usage >= 240 被忽略，
/// timestamp 只写低 48 位，保留字节为零。
[[nodiscard]] std::vector<uint8_t> keyboard_report(const std::vector<uint16_t>& usages, uint64_t timestamp = report_timestamp());

/// 独占一条 universalhidservice 连接，Device 的生命周期必须覆盖本对象。
/// 同一实例的调用由调用方串行安排。发送中途失败会立即返回，不自动补发
/// 抬起或松键报告；需要恢复已经发送的接触/按键状态时，由调用方处理。
class Service
{
public:
    /// RSD 目录缺少服务或建立连接失败时返回 nullptr，并填写 err。
    static std::unique_ptr<Service> open(maa::ios::remote::Device& device, std::string& err, bool verbose = false);

    struct Surface
    {
        uint64_t service_id = 0;
        std::string name;
    };

    /// 枚举设备当前注册的 HID 面，将非零 service_id 的条目追加到 out。
    /// 读取 Product，缺失时使用 ServiceName。已有 USB 设备在目录回复后关闭过
    /// 连接，因此枚举应使用独立实例，注入另开连接；枚举成功不保证该连接仍能发送报告。
    bool surfaces(std::vector<Surface>& out, std::string& err);

    /// 返回 connectedServices 的完整回复，保留 surfaces() 未提取的设备字段。
    /// 此调用与 surfaces() 一样可能结束设备端连接，不应与后续注入共用实例。
    bool raw_connected_services(maa::ios::xpc::Value& reply, std::string& err);

    /// 投递原始报告，首字节为 HID 报告号，service_id 指定目标面。
    /// 默认只发送：已有设备不对正常 send 请求逐条回复。返回 true 仅表示
    /// 本地发送成功，不确认设备接受报告或界面产生变化。
    /// reply 非空时等待最多 5 秒并返回原始回复，供诊断使用；无回复时可能超时，
    /// 不能把该路径的结果单独作为输入是否生效的判据。
    bool send_report(uint64_t service_id, std::span<const uint8_t> report, std::string& err, maa::ios::xpc::Value* reply = nullptr);

    /// 向指定 service_id 发送主触摸屏格式的接触或抬起报告。
    /// x/y 是 0..1 的归一化屏幕坐标；调用方应选择支持该报告格式的面。
    bool touch(uint64_t service_id, double x, double y, bool down, std::string& err);

    /// 在主触摸屏同一坐标按下、等待 hold_ms、抬起；hold_ms <= 0 时不等待。
    bool tap(double x, double y, int hold_ms, std::string& err);

    /// 按点序发送触摸报告：除末点外均为 CONTACT，末点为 RELEASE。
    /// 空列表返回错误；只有一个点时仅发送 RELEASE，不构成点击或拖动。
    /// step_ms > 0 时在相邻报告间等待，配合时间戳表达移动速度；具体间隔的
    /// 适用范围取决于设备手势处理，已有测试记录见 docs/coredevice.md 第 12 节。
    bool stroke(const std::vector<std::pair<double, double>>& points, int step_ms, std::string& err);

    /// 向指定键盘面发送 usages 完整集合，等待 hold_ms 后发送空集合松键。
    /// hold_ms <= 0 时不等待。此函数不拆分修饰键；组合键使用 press_chord()。
    /// surface 是该键盘面的 `_ServiceID`，设备当前面标识可通过独立连接枚举。
    bool type(uint64_t surface, const std::vector<uint16_t>& usages, int hold_ms, std::string& err);

    /// 组合键（如 Command+V）：先单独发送 0xE0..0xE7 范围内的修饰键，
    /// 再发送 usages 完整集合，等待 hold_ms 后全部松开。已有设备测试要求
    /// 修饰键先进入按下状态，同时按下修饰键与主键不能保证组合效果。
    bool press_chord(uint64_t surface, const std::vector<uint16_t>& usages, int hold_ms, std::string& err);

    /// 向设备自带键盘面逐条发送 text_reports() 生成的 ASCII 报告。
    /// hold_ms > 0 时在每条报告后等待，包括修饰键和松键报告。
    /// 文本目标须由调用方保持焦点；本函数不检查焦点，也不确认文本已被接收。
    /// 不支持的字符被跳过，发送成功不表示原始字符串已完整输入。
    bool type_text(const std::string& text, int hold_ms, std::string& err);

private:
    explicit Service(std::unique_ptr<maa::ios::remote::ServiceConnection> conn)
        : conn_(std::move(conn))
    {
    }

    std::unique_ptr<maa::ios::remote::ServiceConnection> conn_;
};

/// 硬件按键的 state 值。
inline constexpr uint64_t kButtonStateDown = 1;
inline constexpr uint64_t kButtonStateUp = 2;
inline constexpr uint64_t kButtonStateCanceled = 3;

/// 硬件按键（home / 锁屏 / 音量 / 静音）。
///
/// 使用 `com.apple.coredevice.hid.indigo` 的 `remote.hid.button` feature。
/// 请求包含 `{messageType, featureIdentifier, payload}`，messageType 为
/// `IndigoButtonEvent`，payload 为 `{state, usagePage, usageCode}`。
/// 该 button 路径已有设备验证；不据此推定 indigo 的 digitizer、keyboard 或
/// scroll 请求可用。连接由本对象独占，Device 必须比本对象活得更久。
class Buttons
{
public:
    static std::unique_ptr<Buttons> open(maa::ios::remote::Device& device, std::string& err, bool verbose = false);

    /// 单独发送 DOWN；重复调用仍发送，由调用方安排最终 UP。
    /// 失败也可能已经部分送达，调用方应使用同一连接尽力释放。
    bool down(uint16_t usage_page, uint16_t usage_code, std::string& err);

    /// 依次发送 DOWN 和 UP，hold_ms > 0 时在两条消息间等待。
    /// state 1=按下、2=抬起、3=取消；本函数仅使用 DOWN/UP。
    /// 返回值表示发送结果，不确认设备动作；中途失败不自动补发 UP。
    bool press(uint16_t usage_page, uint16_t usage_code, int hold_ms, std::string& err);

    /// 单独发送 UP，供调用方在 press() 中途失败后尝试释放。
    /// 返回值仍只表示发送结果；连接断开时无法保证设备收到释放消息。
    bool release(uint16_t usage_page, uint16_t usage_code, std::string& err);

    [[nodiscard]] bool available() const { return conn_ != nullptr; }

private:
    explicit Buttons(std::unique_ptr<maa::ios::remote::ServiceConnection> conn)
        : conn_(std::move(conn))
    {
    }

    bool send(uint64_t state, uint16_t usage_page, uint16_t usage_code, std::string& err);

    std::unique_ptr<maa::ios::remote::ServiceConnection> conn_;
};

/// Consumer page（0x0C）上的硬件按键 usage。
namespace button
{
inline constexpr uint16_t kUsagePageConsumer = 0x0C;
inline constexpr uint16_t kHome = 0x40;
inline constexpr uint16_t kLock = 0x30;
inline constexpr uint16_t kVolumeUp = 0xE9;
inline constexpr uint16_t kVolumeDown = 0xEA;
inline constexpr uint16_t kMute = 0xE2;
} // namespace button

} // namespace maa::ios::hid
