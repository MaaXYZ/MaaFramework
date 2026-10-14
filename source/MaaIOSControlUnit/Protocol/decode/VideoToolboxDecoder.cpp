#include "Decoder.h"
#include "i18n/Translation.h"

#include <CoreMedia/CMBlockBuffer.h>
#include <CoreMedia/CMFormatDescription.h>
#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <VideoToolbox/VideoToolbox.h>

#include <cstdio>
#include <cstring>

namespace maa::ios
{
namespace
{

void store_be16(uint8_t* p, uint16_t v)
{
    p[0] = uint8_t(v >> 8);
    p[1] = uint8_t(v);
}

/// 一次提交对应一个输出槽，通过 source_frame_ref_con 传给输出回调。
/// 回调可能晚于 DecodeFrame 返回，槽必须覆盖回调的生命周期。当前成功提交路径
/// 调用 WaitForAsynchronousFrames 后才读取和释放槽；同一实例由调用方串行使用。
/// 回调 retain 输出像素缓冲，decode 在复制到 Frame 后负责 release。
struct OutputSlot
{
    CVPixelBufferRef pb = nullptr;
};

void output_callback(
    void* ref_con,
    void* source_frame_ref_con,
    OSStatus status,
    VTDecodeInfoFlags,
    CVImageBufferRef image_buffer,
    CMTime,
    CMTime)
{
    (void)ref_con;
    auto* slot = static_cast<OutputSlot*>(source_frame_ref_con);
    if (slot == nullptr || status != noErr || image_buffer == nullptr) {
        return;
    }
    slot->pb = CVPixelBufferRetain(image_buffer);
}

/// 当前适配器使用 2 字节 NAL 长度前缀。已有 macOS 26 / Apple Silicon
/// 样本中，创建参数和写入前缀长度的 2×2 组合测试只有 2 字节组合成功；
/// 4 字节会话返回 kVTVideoDecoderBadDataErr(-12909)。这项观察不代表
/// 其他系统版本或码流也有相同限制。当前后端因此最多接受 65535 字节 NAL。
constexpr int kNalLengthSize = 2;
constexpr size_t kMaxNalSize = 0xFFFF;

class VideoToolboxDecoder final : public Decoder
{
public:
    ~VideoToolboxDecoder() override { teardown(); }

    bool configure(const Nal& vps, const Nal& sps, const Nal& pps) override
    {
        teardown();

        const uint8_t* sets[3] = { vps.data(), sps.data(), pps.data() };
        const size_t sizes[3] = { vps.size(), sps.size(), pps.size() };

        OSStatus st = CMVideoFormatDescriptionCreateFromHEVCParameterSets(nullptr, 3, sets, sizes, kNalLengthSize, nullptr, &format_desc_);
        if (st != noErr) {
            std::fprintf(stderr, MAA_IOS_TR("Failed to parse HEVC parameter sets: %d\n"), static_cast<int>(st));
            return false;
        }

        // 属性值使用 CFNumber 对象。字典通过 CF 类型回调保留该对象，创建字典后
        // 可释放本地引用；不能把整数地址当作 CFNumberRef 传入。
        const uint32_t bgra = kCVPixelFormatType_32BGRA;
        CFNumberRef bgra_num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &bgra);
        if (bgra_num == nullptr) {
            teardown();
            return false;
        }
        const void* keys[] = { kCVPixelBufferPixelFormatTypeKey };
        const void* vals[] = { bgra_num };
        CFDictionaryRef dest = CFDictionaryCreate(nullptr, keys, vals, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFRelease(bgra_num);
        if (dest == nullptr) {
            teardown();
            return false;
        }

        const VTDecompressionOutputCallbackRecord cb { output_callback, nullptr };
        st = VTDecompressionSessionCreate(nullptr, format_desc_, nullptr, dest, &cb, &session_);
        CFRelease(dest);

        if (st != noErr || session_ == nullptr) {
            std::fprintf(stderr, MAA_IOS_TR("Failed to create VideoToolbox session: %d\n"), static_cast<int>(st));
            teardown();
            return false;
        }
        return true;
    }

    bool decode(const std::vector<Nal>& au, Frame& out) override
    {
        if (session_ == nullptr) {
            return false;
        }

        // 当前适配器仅将 VCL NAL 写入样本，参数集来自 configure 的 format
        // description。已有样本中重复内联参数集会导致 bad data，不外推其他配置。
        std::vector<const Nal*> slices;
        slices.reserve(au.size());
        size_t total = 0;
        for (const auto& n : au) {
            if (n.size() < 2) {
                continue;
            }
            const uint8_t type = static_cast<uint8_t>((n[0] >> 1) & 0x3F);
            if (type >= 32) {
                continue; // 非 VCL：参数集 / SEI / 保留
            }
            if (n.size() > kMaxNalSize) {
                warn_oversized(n.size());
                return false;
            }
            slices.push_back(&n);
            total += kNalLengthSize + n.size();
        }
        if (total == 0) {
            return false;
        }

        std::vector<uint8_t> buf;
        buf.reserve(total);
        // 每个 VCL NAL 前写两字节大端长度，长度仅计 NAL 本身，不含前缀。
        // NAL 保留 emulation prevention 字节，不能先转成去转义的 RBSP；长度和
        // 样本内容均按 AnnexB.h 中原始 Nal 的语义提交。
        for (const Nal* n : slices) {
            uint8_t len[2];
            store_be16(len, static_cast<uint16_t>(n->size()));
            buf.insert(buf.end(), len, len + kNalLengthSize);
            buf.insert(buf.end(), n->begin(), n->end());
        }

        // CoreMedia 分配独立存储，再将 buf 复制进去；样本不引用局部 vector 的内存。
        CMBlockBufferRef bb = nullptr;
        OSStatus st = CMBlockBufferCreateWithMemoryBlock(
            nullptr,
            nullptr,
            buf.size(),
            kCFAllocatorDefault,
            nullptr,
            0,
            buf.size(),
            kCMBlockBufferAssureMemoryNowFlag,
            &bb);
        if (st != kCMBlockBufferNoErr) {
            return false;
        }
        st = CMBlockBufferReplaceDataBytes(buf.data(), bb, 0, buf.size());
        if (st != kCMBlockBufferNoErr) {
            CFRelease(bb);
            return false;
        }

        CMSampleTimingInfo timing { };
        timing.duration = kCMTimeInvalid;
        timing.decodeTimeStamp = kCMTimeInvalid;
        timing.presentationTimeStamp = CMTimeMake(pts_num_++, 1000);

        const size_t sample_size = buf.size();
        CMSampleBufferRef sb = nullptr;
        st = CMSampleBufferCreateReady(nullptr, bb, format_desc_, 1, 1, &timing, 1, &sample_size, &sb);
        // 成功创建的 sample buffer 保留 block buffer，本地引用可在此释放。
        CFRelease(bb);
        if (st != noErr || sb == nullptr) {
            return false;
        }

        OutputSlot slot;
        st = VTDecompressionSessionDecodeFrame(session_, sb, 0, &slot, nullptr);
        CFRelease(sb);
        if (st != noErr) {
            if (slot.pb != nullptr) {
                CVPixelBufferRelease(slot.pb);
            }
            return false;
        }
        // 等待已提交帧的异步回调完成，之后才读取 slot 并结束其栈上生命周期。
        // 当前适配器每次提交后等待，不在多个 decode 调用之间建立异步流水。
        VTDecompressionSessionWaitForAsynchronousFrames(session_);
        if (slot.pb == nullptr) {
            return false;
        }
        copy_out(slot.pb, out);
        CVPixelBufferRelease(slot.pb);
        slot.pb = nullptr;
        return static_cast<bool>(out);
    }

    [[nodiscard]] const char* backend_name() const override { return "VideoToolbox"; }

    /// 当前两字节 NAL 长度前缀的表示上限；调用方可据此选择软件后端。
    [[nodiscard]] size_t max_nal_size() const override { return kMaxNalSize; }

private:
    void teardown()
    {
        if (session_ != nullptr) {
            VTDecompressionSessionInvalidate(session_);
            CFRelease(session_);
            session_ = nullptr;
        }
        if (format_desc_ != nullptr) {
            CFRelease(format_desc_);
            format_desc_ = nullptr;
        }
    }

    /// 当前两字节前缀不能表示大于 65535 字节的 NAL。已有设备样本出现过
    /// 超限关键帧，调用方需改用软件解码；不要依据某个小码流样本假定不会触发。
    static void warn_oversized(size_t n)
    {
        static bool warned = false;
        if (!warned) {
            warned = true;
            std::fprintf(
                stderr,
                MAA_IOS_TR(
                    "NAL size %zu exceeds the 2-byte length prefix; frame dropped. This stream "
                    "requires software decoding.\n"),
                n);
        }
    }

    /// 锁定像素缓冲后按源行跨度读取，复制为紧凑 BGRA；Frame 拥有复制后的像素。
    /// 锁定失败或源数据无效时不更新 out，调用方不能将旧内容视为本次新输出。
    static void copy_out(CVPixelBufferRef pb, Frame& out)
    {
        if (CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) {
            return;
        }
        const auto* src = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pb));
        const size_t width = CVPixelBufferGetWidth(pb);
        const size_t rows = CVPixelBufferGetHeight(pb);
        const size_t src_pitch = CVPixelBufferGetBytesPerRow(pb);
        const size_t row_bytes = width * 4;

        if (src != nullptr && rows > 0 && row_bytes > 0) {
            out.width = static_cast<uint32_t>(width);
            out.height = static_cast<uint32_t>(rows);
            out.row_pitch = static_cast<uint32_t>(row_bytes);
            out.bytes_per_pixel = 4;
            out.pixels.resize(row_bytes * rows);
            for (size_t r = 0; r < rows; ++r) {
                std::memcpy(out.pixels.data() + r * row_bytes, src + r * src_pitch, row_bytes);
            }
        }
        CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
    }

    CMFormatDescriptionRef format_desc_ = nullptr;
    VTDecompressionSessionRef session_ = nullptr;
    int64_t pts_num_ = 0;
};

} // namespace

std::unique_ptr<Decoder> create_platform_decoder()
{
    return std::make_unique<VideoToolboxDecoder>();
}

} // namespace maa::ios
