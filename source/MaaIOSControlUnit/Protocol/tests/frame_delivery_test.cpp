#include "media/FramePump.h"
#include "media/StreamSession.h"
#include "rt/Rtcp.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string_view>
#include <thread>

#ifdef MAA_IOS_HAVE_LIBAVFORMAT
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}
#endif

// 运行真实 FramePump、RTP/AU/参数及 IDR 检查、Recorder。仅在本可执行文件
// 替换已有 StreamSession 网络边界及 Decoder 工厂，不造设备服务器或测试私有状态。
namespace
{
using Pump = maa::ios::media::FramePump;
using Session = maa::ios::media::StreamSession;
using Bytes = maa::ios::Nal;
using namespace std::chrono_literals;
int checks = 0, failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", message);
    }
}

struct Script
{
    std::mutex mutex;
    std::condition_variable ready;
    std::deque<Bytes> packets;
    std::vector<Session::Request> starts;
    std::vector<std::pair<Bytes, uint16_t>> sent;
    std::atomic<unsigned> factories { 0 }, configured { 0 }, decoded { 0 }, max_nal_calls { 0 }, stops { 0 }, probes { 0 };
    std::atomic<bool> decoder_available { false }, pixels { false };
    bool local_present = true, send_ok = true;
    uint32_t local = 0;
    std::optional<Session::StartStatus> fail_next;

    void reset()
    {
        std::lock_guard lock(mutex);
        packets.clear();
        starts.clear();
        sent.clear();
        fail_next.reset();
        factories = configured = decoded = max_nal_calls = stops = probes = 0;
        decoder_available = pixels = false;
        local_present = true;
        local = 0;
        send_ok = true;
    }

    void enqueue(Bytes packet)
    {
        {
            std::lock_guard lock(mutex);
            packets.push_back(std::move(packet));
        }
        ready.notify_one();
    }

    std::size_t start_count()
    {
        std::lock_guard lock(mutex);
        return starts.size();
    }
} script;

template <class Predicate>
bool wait(Predicate predicate, std::chrono::milliseconds limit = 2000ms)
{
    const auto until = std::chrono::steady_clock::now() + limit;
    do {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(5ms);
    } while (std::chrono::steady_clock::now() < until);
    return predicate();
}

void put32(Bytes& bytes, std::size_t offset, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        bytes[offset + i] = static_cast<uint8_t>(value >> (24 - 8 * i));
    }
}

Bytes hex(std::string_view text)
{
    Bytes bytes;
    for (std::size_t i = 0; i < text.size(); i += 2) {
        unsigned value = 0;
        std::sscanf(text.data() + i, "%2x", &value);
        bytes.push_back(static_cast<uint8_t>(value));
    }
    return bytes;
}

// 固定 FFmpeg/x265 64x64、bframes=0 视频样例。
const auto vps = hex("40010c01ffff01600000030090000003000003001eba0240");
const auto sps = hex("42010101600000030090000003000003001ea020810596e92930bc05a020000003002000000303c1");
const auto pps = hex("4401c073c089");
const auto idr = hex("2801ac21800e7ffeebf349ac");

Bytes rtp(uint16_t sequence, uint32_t ticks, const Bytes& payload, bool marker = false, uint32_t source = 0, uint8_t type = 101)
{
    Bytes packet(12, 0);
    packet[0] = 0x80;
    packet[1] = static_cast<uint8_t>(type | (marker ? 0x80 : 0));
    packet[2] = static_cast<uint8_t>(sequence >> 8);
    packet[3] = static_cast<uint8_t>(sequence);
    put32(packet, 4, ticks);
    put32(packet, 8, source);
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

uint16_t parameters(uint16_t sequence, uint32_t ticks = 0, uint32_t source = 0)
{
    for (const auto* nal : { &vps, &sps, &pps }) {
        script.enqueue(rtp(sequence++, ticks, *nal, false, source));
    }
    return sequence;
}

class Decoder final : public maa::ios::Decoder
{
public:
    bool configure(const Bytes&, const Bytes&, const Bytes&) override
    {
        ++script.configured;
        return true;
    }

    bool decode(const std::vector<Bytes>&, maa::ios::Frame& frame) override
    {
        ++script.decoded;
        if (!script.pixels) {
            return false;
        }
        frame.width = frame.height = 64;
        frame.row_pitch = 256;
        frame.pixels.assign(64u * 256u, 17);
        return true;
    }

    std::size_t max_nal_size() const override
    {
        ++script.max_nal_calls;
        return ~std::size_t { 0 };
    }

    const char* backend_name() const override { return "controlled"; }
};

void decoded_frames()
{
    script.reset();
    maa::ios::remote::Device device;
    std::string error;
    script.decoder_available = true;
    Pump::Options options;
    std::atomic<int> orientation { 0 };
    options.frame_geometry = [&] {
        return std::optional(maa::ios::FrameGeometry { 64, 64, orientation == 0 ? "portrait" : "landscapeRight" });
    };
    auto pump = Pump::start(device, options, error);
    check(pump != nullptr, "decoded video starts");
    if (!pump) {
        return;
    }
    auto sequence = parameters(10);
    script.enqueue(rtp(sequence++, 0, idr, true));
    check(
        wait([] { return script.decoded == 1; }) && !pump->wait_ready(0) && pump->serial() == 0,
        "syntax alone never publishes a ready frame");
    script.pixels = true;
    script.enqueue(rtp(sequence++, 2400, idr, true));
    check(pump->wait_ready(2000), "real decoded pixels make video ready");
    maa::ios::Frame first;
    const auto serial = pump->newer(first, 0, 2000);
    check(
        serial && first.width == 64 && first.height == 64 && first.geometry && first.geometry->orientation == "portrait",
        "display metadata travels with decoded pixels");
    orientation = 90;
    maa::ios::Frame cached;
    check(
        pump->latest(cached, 0) && cached.geometry && cached.geometry->orientation == "portrait",
        "new display state does not reinterpret an old cached frame");
    script.enqueue(rtp(sequence++, 4800, idr, true));
    maa::ios::Frame next;
    check(
        pump->newer(next, serial, 2000) > serial && next.geometry && next.geometry->orientation == "landscapeRight",
        "new decoded frame captures changed display state");
    ++sequence;
    script.enqueue(rtp(sequence++, 7200, Bytes { 0x02, 0x01, 0x80, 0x00 }, true));
    check(wait([&] { return pump->stats().gaps > 0; }), "sequence loss reaches real video recovery");
    check(wait([&] { return pump->stats().pli_sent > 0; }), "loss requests a replacement keyframe");
    script.enqueue(rtp(sequence++, 9600, idr, true));
    check(wait([&] { return pump->stats().decoded >= 3 && !pump->reviving(); }), "new keyframe restores decoded video after loss");
    pump.reset();
    check(script.stops == 1, "destruction joins video worker and stops the owned session");
}
} // namespace

namespace maa::ios
{
std::unique_ptr<Decoder> create_software_decoder()
{
    ++script.factories;
    return script.decoder_available ? std::make_unique<::Decoder>() : nullptr;
}

std::unique_ptr<Decoder> create_platform_decoder()
{
    ++script.factories;
    return script.decoder_available ? std::make_unique<::Decoder>() : nullptr;
}
} // namespace maa::ios

namespace maa::ios::media
{
std::unique_ptr<StreamSession>
    StreamSession::start(remote::Device&, const Request& request, std::string& error, bool, remote::ServiceConnection*, StartStatus* status)
{
    std::lock_guard lock(script.mutex);
    script.starts.push_back(request);
    if (script.fail_next) {
        if (status) {
            *status = *script.fail_next;
        }
        script.fail_next.reset();
        error = "controlled video negotiation failure";
        return nullptr;
    }
    Started started;
    started.sender_port = 24680;
    started.payload_type = 101;
    started.local_ssrc = script.local;
    started.has_local_ssrc = script.local_present;
    started.remote_ssrc = 123;
    started.has_remote_ssrc = true;
    started.session_uuid.assign(16, static_cast<uint8_t>(script.starts.size()));
    error.clear();
    if (status) {
        *status = StartStatus::Started;
    }
    return std::unique_ptr<StreamSession>(new StreamSession(nullptr, std::move(started)));
}

StreamSession::~StreamSession() = default;

uint16_t StreamSession::receiver_port() const
{
    return 49152;
}

bool StreamSession::next_packet(Bytes& packet, int timeout, std::string& error)
{
    std::unique_lock lock(script.mutex);
    if (!script.ready.wait_for(lock, std::chrono::milliseconds(timeout), [] { return !script.packets.empty(); })) {
        error = "controlled timeout";
        return false;
    }
    packet = std::move(script.packets.front());
    script.packets.pop_front();
    error.clear();
    return true;
}

bool StreamSession::send_rtp(const Bytes& payload, uint16_t port, std::string& error)
{
    std::lock_guard lock(script.mutex);
    script.sent.emplace_back(payload, port);
    error = script.send_ok ? "" : "controlled send failure";
    return script.send_ok;
}

bool StreamSession::stop(remote::Device&, std::string& error, bool) const
{
    ++script.stops;
    error.clear();
    return true;
}

StreamSession::ServerState StreamSession::probe(remote::Device&, const Bytes&, std::string& error, bool)
{
    ++script.probes;
    error.clear();
    return ServerState::Alive;
}
} // namespace maa::ios::media

int main()
{
    decoded_frames();
    std::printf("frame_delivery: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
