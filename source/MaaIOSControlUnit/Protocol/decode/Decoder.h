#pragma once


#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "bitstream/AnnexB.h"

namespace maa::ios
{

/// 一帧 CPU 侧可读的 BGRA 图像。
///
/// pixels 拥有图像数据，释放后端的输出缓冲后仍可读取。当前后端将像素复制或
/// 转换到此缓冲；row_pitch 表示目标行跨度，调用方不能按后端的源行跨度寻址。
/// 当前输出为每像素 4 字节的 BGRA，不包含后端的纹理或像素缓冲句柄。
struct FrameGeometry
{
    int display_width = 0;
    int display_height = 0;
    // Preserve unknown orientations instead of interpreting them as portrait.
    std::string orientation;
};

struct Frame
{
    std::optional<FrameGeometry> geometry;
    std::vector<uint8_t> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t row_pitch = 0; ///< 一行占多少字节，可能大于 width*4

    uint32_t bytes_per_pixel = 4;

    [[nodiscard]] explicit operator bool() const { return width > 0 && height > 0; }

    [[nodiscard]] uint32_t display_width() const { return width; }
};

class Decoder
{
public:
    virtual ~Decoder() = default;

    /// 用 VPS、SPS、PPS 建立或重建解码会话；参数集不含 Annex-B 起始码。
    /// 当前后端会先释放旧会话，失败后不能假定旧会话仍可继续使用。
    /// 同一实例的 configure、decode 和销毁由调用方串行执行。
    virtual bool configure(const Nal& vps, const Nal& sps, const Nal& pps) = 0;

    /// 提交一个 access unit，成功取得图像时返回 true。
    /// 返回 false 既可能表示暂时无输出，也可能表示输入、解码或转换失败；此接口
    /// 不区分这些原因，后续恢复由调用方处理。失败路径不统一清空 out，不能据其
    /// 原有像素判断本次成功；调用方应同时检查返回值和 Frame 的有效尺寸。
    virtual bool decode(const std::vector<Nal>& au, Frame& out) = 0;

    /// 当前适配器接受的单个 NAL 字节数上限，不含起始码或长度前缀。
    ///
    /// 调用方取得完整 AU 后可据此选择后端。当前 VideoToolbox 适配器使用两字节
    /// 长度前缀，上限为 65535；已有设备样本含 70101 字节的 NAL，需软件解码。
    /// 这项限制来自当前适配配置，不能推断所有 VideoToolbox 会话都只能用两字节。
    /// 默认 SIZE_MAX 表示接口未声明此项上限，不免除底层库或总缓冲大小的约束。
    [[nodiscard]] virtual size_t max_nal_size() const { return ~size_t { 0 }; }

    [[nodiscard]] virtual const char* backend_name() const = 0;
};

/// 当前平台的默认后端：Apple 构建创建 VideoToolbox，其他构建转入软件工厂。
/// 工厂返回对象不代表 configure 一定成功，也不保证使用硬件解码。
std::unique_ptr<Decoder> create_platform_decoder();

/// 软件解码后端（libavcodec）。未编入此后端时返回 nullptr，调用方需处理空结果。
/// 当前项目以它承接超出 VideoToolbox 适配器 NAL 长度上限的输入，并将它作为
/// 非 Apple 平台的默认后端；是否具备 HEVC 解码器仍需 configure 检查。
std::unique_ptr<Decoder> create_software_decoder();

/// 本构建是否编入至少一种 HEVC 解码后端，可供调用方在起流前检查。
/// Apple 构建编入 VideoToolbox；非 Apple 构建仅在 MAA_IOS_HAVE_LIBAV 时有后端。
/// true 表示编译时能力，不保证运行时会话创建或具体码流解码成功。
#if defined(__APPLE__) || defined(MAA_IOS_HAVE_LIBAV)
inline constexpr bool kHaveDecoder = true;
#else
inline constexpr bool kHaveDecoder = false;
#endif

/// 本构建未编入 HEVC 后端时的共用提示。
inline constexpr const char* kNoDecoderMessage =
    "No HEVC decoder in this build. Non-Apple platforms require FFmpeg development "
    "packages. Install them and reconfigure (Debian/Ubuntu: apt install "
    "libavcodec-dev libavutil-dev libswscale-dev; Fedora: dnf install "
    "ffmpeg-devel).\n";

} // namespace maa::ios
