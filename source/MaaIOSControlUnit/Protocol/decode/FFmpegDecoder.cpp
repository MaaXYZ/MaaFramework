#include "decode/Decoder.h"
#include "i18n/Translation.h"

// 以 C 链接约定声明 libav 接口，与其库导出的符号一致。
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <cstdio>
#include <cstring>
#include <string>

namespace maa::ios
{
namespace
{

/// 将 libav 错误码转换为诊断文本。
std::string av_strerr(int st)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_strerror(st, buf, sizeof(buf));
    return buf;
}

/// libavcodec HEVC 软件后端。输入转换为 Annex-B，输出经 swscale 转为 CPU BGRA。
/// 当前项目将其用于非 Apple 平台，以及超出 VideoToolbox 适配器 NAL 上限的码流。
/// ctx_、frame_、sws_ 和参数集缓存由实例拥有，同一实例不提供并发调用保护。
class FFmpegDecoder final : public Decoder
{
public:
    ~FFmpegDecoder() override { teardown(); }

    bool configure(const Nal& vps, const Nal& sps, const Nal& pps) override
    {
        teardown();

        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
        if (codec == nullptr) {
            std::fprintf(stderr, MAA_IOS_TR("FFmpeg has no HEVC decoder\n"));
            return false;
        }
        ctx_ = avcodec_alloc_context3(codec);
        if (ctx_ == nullptr) {
            return false;
        }
        // 请求低延迟解码；已有设备样本未观察到 B 帧。此标志不保证每次提交
        // 都产生输出，也不能替代解码器自身的参考帧和线程调度要求。
        ctx_->flags |= AV_CODEC_FLAG_LOW_DELAY;
        // 由 libavcodec 自动选择线程数；这是库内部并行，不允许调用方并发操作 ctx_。
        ctx_->thread_count = 0;
        // 打开解码上下文后才允许提交 packet；参数集将在 decode 中随 AU 提交。
        const int open_st = avcodec_open2(ctx_, codec, nullptr);
        if (open_st < 0) {
            std::fprintf(stderr, MAA_IOS_TR("Failed to open FFmpeg HEVC decoder: %s\n"), av_strerr(open_st).c_str());
            teardown();
            return false;
        }

        // 保存 configure 的参数集，用于 AU 未携带任何 VPS/SPS/PPS 时补充输入。
        sets_.clear();
        append_nal(sets_, vps);
        append_nal(sets_, sps);
        append_nal(sets_, pps);

        frame_ = av_frame_alloc();
        if (frame_ == nullptr) {
            teardown();
            return false;
        }
        return true;
    }

    bool decode(const std::vector<Nal>& au, Frame& out) override
    {
        if (ctx_ == nullptr) {
            return false;
        }
        std::vector<uint8_t> annexb;
        if (!has_parameter_sets(au)) {
            annexb = sets_; // 未发现 VPS/SPS/PPS，先添加 configure 时缓存的参数集
        }
        for (const auto& n : au) {
            append_nal(annexb, n);
        }
        if (annexb.empty()) {
            return false; // 未形成可提交的 Annex-B 输入
        }

        AVPacket* pkt = av_packet_alloc();
        if (pkt == nullptr) {
            return false;
        }
        // av_new_packet 分配 packet 自有数据及 AV_INPUT_BUFFER_PADDING_SIZE 零填充，
        // 满足 libav 位读取器对输入尾部的要求。annexb 的 vector 无需保留到解码结束。
        if (av_new_packet(pkt, static_cast<int>(annexb.size())) < 0) {
            av_packet_free(&pkt);
            return false;
        }
        std::memcpy(pkt->data, annexb.data(), annexb.size());

        const int send_st = avcodec_send_packet(ctx_, pkt);
        av_packet_free(&pkt);
        if (send_st < 0) {
            static bool warned = false;
            if (!warned) {
                warned = true;
                std::fprintf(stderr, MAA_IOS_TR("FFmpeg rejected packet: %s\n"), av_strerr(send_st).c_str());
            }
            return false;
        }

        av_frame_unref(frame_);
        const int st = avcodec_receive_frame(ctx_, frame_);
        if (st < 0) {
            return false; // 包括 EAGAIN（暂时无帧）及其他接收错误，接口均返回 false
        }
        return to_frame(frame_, out);
    }

    [[nodiscard]] const char* backend_name() const override { return "libavcodec/hevc"; }

private:
    static bool has_parameter_sets(const std::vector<Nal>& au)
    {
        for (const auto& n : au) {
            if (n.size() >= 2) {
                const int type = (n[0] >> 1) & 0x3F;
                if (type == 32 || type == 33 || type == 34) { // VPS / SPS / PPS
                    return true;
                }
            }
        }
        return false;
    }

    /// 添加四字节 Annex-B 起始码及原始 NAL，保留 emulation prevention 字节。
    /// decode 仅在 AU 不含任何 VPS/SPS/PPS 时添加缓存；一旦发现其中任一种参数集，
    /// 就按 AU 原样提交，不用缓存补齐或覆盖。此判断不验证三类参数集是否齐全。
    static void append_nal(std::vector<uint8_t>& out, const Nal& n)
    {
        const uint8_t sc[4] = { 0, 0, 0, 1 };
        out.insert(out.end(), sc, sc + 4);
        out.insert(out.end(), n.begin(), n.end());
    }

    bool to_frame(const AVFrame* f, Frame& out)
    {
        const int w = f->width;
        const int h = f->height;
        const AVPixelFormat src = static_cast<AVPixelFormat>(f->format);
        const AVPixFmtDescriptor* desc = src == AV_PIX_FMT_NONE ? nullptr : av_pix_fmt_desc_get(src);
        if (w <= 0 || h <= 0 || desc == nullptr || (desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
            std::fprintf(
                stderr,
                MAA_IOS_TR("Software decoder requires CPU frames; received format=%d %dx%d\n"),
                static_cast<int>(src),
                w,
                h);
            return false;
        }

        const auto row_bytes = static_cast<uint32_t>(w) * 4;
        // 输出缓冲按紧凑 BGRA 行布局分配，仅在所需字节数变化时 resize。
        // 同尺寸帧复用已有存储，由 sws_scale 写入像素。
        const size_t need = static_cast<size_t>(row_bytes) * static_cast<size_t>(h);
        if (out.pixels.size() != need) {
            out.pixels.resize(need);
        }
        out.width = static_cast<uint32_t>(w);
        out.height = static_cast<uint32_t>(h);
        out.row_pitch = row_bytes;
        out.bytes_per_pixel = 4;

        uint8_t* dst_data[4] = { out.pixels.data(), nullptr, nullptr, nullptr };
        const int dst_linesize[4] = { static_cast<int>(row_bytes), 0, 0, 0 };
        sws_ = sws_getCachedContext(sws_, w, h, src, w, h, AV_PIX_FMT_BGRA, SWS_POINT, nullptr, nullptr, nullptr);
        if (sws_ == nullptr || sws_scale(sws_, f->data, f->linesize, 0, h, dst_data, dst_linesize) <= 0) {
            out = Frame { };
            return false;
        }
        return true;
    }

    void teardown()
    {
        if (sws_ != nullptr) {
            sws_freeContext(sws_);
            sws_ = nullptr;
        }
        if (frame_ != nullptr) {
            av_frame_free(&frame_);
            frame_ = nullptr;
        }
        if (ctx_ != nullptr) {
            avcodec_free_context(&ctx_);
            ctx_ = nullptr;
        }
    }

    AVCodecContext* ctx_ = nullptr;
    AVFrame* frame_ = nullptr;
    SwsContext* sws_ = nullptr;
    std::vector<uint8_t> sets_;
};

} // namespace

std::unique_ptr<Decoder> create_software_decoder()
{
    // 将 libav 的进程级日志阈值设为 error，抑制已测全范围 yuvj420p 输入的重复
    // 像素格式警告。这不改变颜色转换配置，也会影响进程内其他 libav 使用者。
    // 本文件直接写 stderr 的诊断不受此阈值影响。
    static bool log_quieted = false;
    if (!log_quieted) {
        av_log_set_level(AV_LOG_ERROR);
        log_quieted = true;
    }
    return std::make_unique<FFmpegDecoder>();
}

} // namespace maa::ios
