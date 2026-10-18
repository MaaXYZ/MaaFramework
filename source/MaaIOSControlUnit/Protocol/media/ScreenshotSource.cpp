#include "media/ScreenshotSource.h"

#include <chrono>
#include <utility>

#include "remote/Device.h"
#include "remote/Rsd.h"
#include "xpc/XpcValue.h"

#ifdef MAA_IOS_HAVE_LIBAV
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
#endif

namespace maa::ios::media
{
namespace
{

constexpr const char* kService = "com.apple.coredevice.screencaptureservice";
constexpr const char* kFeature = "com.apple.coredevice.feature.capturescreenshot";
constexpr const char* kAction = "com.apple.coredevice.action.capturescreenshot";

/// 截图失败后等待再重试，避免设备持续拒绝时频繁发请求。
constexpr int kRetryBackoffMs = 250;
/// 单次截图 invoke 的超时，限制设备无响应时的等待。
/// 连接建立耗时另由传输层控制，因此不是整个 capture_once 或析构的总上限。
constexpr int kCaptureTimeoutMs = 5000;

} // namespace

bool decode_png_bgra(const std::vector<uint8_t>& png, maa::ios::Frame& out, std::string& err)
{
#ifdef MAA_IOS_HAVE_LIBAV
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_PNG);
    if (codec == nullptr) {
        err = "FFmpeg has no PNG decoder";
        return false;
    }
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool ok = false;
    if (ctx != nullptr && pkt != nullptr && frame != nullptr) {
        pkt->data = const_cast<uint8_t*>(png.data());
        pkt->size = static_cast<int>(png.size());
        if (avcodec_open2(ctx, codec, nullptr) == 0 && avcodec_send_packet(ctx, pkt) == 0 && avcodec_receive_frame(ctx, frame) == 0) {
            const int w = frame->width;
            const int h = frame->height;
            // PNG 像素格式可能随内容变化，统一通过 libswscale 转换为 BGRA，
            // 不将输入限定为单一格式。
            if (w > 0 && h > 0) {
                SwsContext* sws = sws_getContext(
                    w,
                    h,
                    static_cast<AVPixelFormat>(frame->format),
                    w,
                    h,
                    AV_PIX_FMT_BGRA,
                    SWS_POINT,
                    nullptr,
                    nullptr,
                    nullptr);
                if (sws != nullptr) {
                    out.width = static_cast<uint32_t>(w);
                    out.height = static_cast<uint32_t>(h);
                    out.row_pitch = static_cast<uint32_t>(w) * 4;
                    out.bytes_per_pixel = 4;
                    out.pixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
                    uint8_t* dst = out.pixels.data();
                    int dst_linesize = w * 4;
                    sws_scale(sws, frame->data, frame->linesize, 0, h, &dst, &dst_linesize);
                    sws_freeContext(sws);
                    ok = true;
                }
                else {
                    err = "Cannot create BGRA conversion (format="
                          + std::string(av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format))) + ")";
                }
            }
            else {
                err = "PNG dimensions must be positive";
            }
        }
        else {
            err = "FFmpeg could not decode PNG image";
        }
    }
    else {
        err = "Failed to allocate PNG decode resources";
    }
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
    return ok;
#else
    (void)png;
    (void)out;
    err = "FFmpeg is not included; screenshot mode has no PNG decoder";
    return false;
#endif
}

ScreenshotSource::ScreenshotSource(remote::Device& device)
    : device_(device)
{
}

ScreenshotSource::~ScreenshotSource()
{
    request_stop();
    // request_stop() 不等待可能正在 RPC 中的 worker，避免阻塞渲染线程。
    // 调用方保留退役对象，在 worker_done 后或应用退出时析构并 join。
    if (worker_.joinable()) {
        worker_.join();
    }
}

std::unique_ptr<ScreenshotSource> ScreenshotSource::start(remote::Device& device, std::string& err, bool capture_first)
{
    if (!device.rsd().has_service(kService)) {
        err = "Device directory missing service: " + std::string(kService);
        return nullptr;
    }
    std::unique_ptr<ScreenshotSource> src(new ScreenshotSource(device));
    // capture_first 时同步取得首张截图，失败直接返回原因。
    // 运行中的来源切换传 false，由工作线程取首帧，避免阻塞窗口。
    if (capture_first) {
        std::vector<uint8_t> png;
        if (!src->capture_once(png, err)) {
            return nullptr;
        }
        maa::ios::Frame first;
        if (!decode_png_bgra(png, first, err)) {
            return nullptr;
        }
        {
            std::lock_guard<std::mutex> lk(src->mu_);
            src->frame_ = std::move(first);
            src->serial_ = 1;
            src->stats_.frames = 1;
            src->stats_.bytes = png.size();
        }
    }
    src->worker_ = std::thread([raw = src.get()] { raw->loop(); });
    return src;
}

bool ScreenshotSource::capture_once(std::vector<uint8_t>& png, std::string& err)
{
    // 每张截图使用新连接，真机观察到此服务复用连接后后续请求失败。
    auto conn = device_.connect(kService, err, false);
    if (conn == nullptr) {
        return false;
    }
    auto input = xpc::make_dict();
    xpc::dict_set(input, "displayUniqueID", xpc::make_null());
    xpc::dict_set(input, "requestedFormat", xpc::make_string("png"));
    xpc::Value out;
    // 限制此次 invoke 的等待，失败后由工作循环退避重试。
    if (conn->invoke(kFeature, kAction, input, out, kCaptureTimeoutMs, err) != remote::CallResult::Ok) {
        return false;
    }
    const auto* image = out.find("image");
    if (image == nullptr || image->data.empty()) {
        err = "Screenshot response missing image data";
        return false;
    }
    png = image->data;
    return true;
}

void ScreenshotSource::loop()
{
    while (!stopping_) {
        std::vector<uint8_t> png;
        std::string err;
        if (!capture_once(png, err)) {
            {
                std::lock_guard<std::mutex> lk(mu_);
                ++stats_.failures;
            }
            cv_.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(kRetryBackoffMs));
            continue;
        }
        maa::ios::Frame f;
        if (!decode_png_bgra(png, f, err)) {
            {
                std::lock_guard<std::mutex> lk(mu_);
                ++stats_.failures;
            }
            cv_.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(kRetryBackoffMs));
            continue;
        }
        {
            std::lock_guard<std::mutex> lk(mu_);
            frame_ = std::move(f);
            ++serial_;
            ++stats_.frames;
            stats_.bytes += png.size();
        }
        cv_.notify_all();
    }
    // 发布线程完成标记，供 app::reap_finished 回收。release 与
    // worker_done() 的 acquire 配对；析构随后 join。
    worker_done_.store(true, std::memory_order_release);
}

bool ScreenshotSource::latest(maa::ios::Frame& out, uint64_t& serial, int timeout_ms)
{
    std::unique_lock<std::mutex> lk(mu_);
    const uint64_t want = serial;
    cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&] { return serial_ > want || stopping_; });
    if (serial_ <= want) {
        return false;
    }
    out = frame_;
    serial = serial_;
    return true;
}

void ScreenshotSource::request_stop()
{
    // 停止标记及唤醒可重复执行，始终通知 latest() 等待者，不在这里 join。
    stopping_ = true;
    cv_.notify_all();
}

ScreenshotSource::Stats ScreenshotSource::stats() const
{
    std::lock_guard<std::mutex> lk(mu_);
    return stats_;
}

void ScreenshotSource::image_size(int& w, int& h) const
{
    std::lock_guard<std::mutex> lk(mu_);
    w = static_cast<int>(frame_.width);
    h = static_cast<int>(frame_.height);
}

} // namespace maa::ios::media
