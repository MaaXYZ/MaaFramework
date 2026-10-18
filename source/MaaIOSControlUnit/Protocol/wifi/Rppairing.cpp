#include "wifi/Rppairing.h"
#include "i18n/Translation.h"

#include <cstring>
#include <utility>

namespace maa::ios::wifi
{

json::Value j_str(std::string_view s)
{
    return std::string(s);
}

json::Value j_int(int64_t v)
{
    return v;
}

json::Value j_bool(bool v)
{
    return v;
}

json::Value j_arr(std::vector<json::Value> items)
{
    return json::Value(std::move(items));
}

json::Value j_obj(std::vector<std::pair<std::string, json::Value>> kv)
{
    auto out = ::json::object { };
    for (auto& [k, v] : kv) {
        out.emplace(std::move(k), std::move(v));
    }
    return out;
}

void Rppairing::install_main_keys(Bytes client_key, Bytes server_key)
{
    client_main_ = std::move(client_key);
    server_main_ = std::move(server_key);
}

bool FramedCarrier::write_envelope(const json::Value& envelope, std::string& err)
{
    const std::string text = json::write(envelope);
    if (text.size() > 0xFFFF) {
        err = MAA_IOS_TR("Frame exceeds u16 length field");
        return false;
    }
    Bytes buf;
    buf.reserve(kRpPairingMagic.size() + 2 + text.size());
    buf.insert(buf.end(), kRpPairingMagic.begin(), kRpPairingMagic.end());
    buf.push_back(static_cast<uint8_t>(text.size() >> 8));
    buf.push_back(static_cast<uint8_t>(text.size() & 0xFF));
    buf.insert(buf.end(), text.begin(), text.end());
    return io_.write_all(buf.data(), buf.size(), err);
}

std::optional<json::Value> FramedCarrier::read_envelope(std::string& err)
{
    uint8_t header[kRpPairingMagic.size() + 2];
    if (!io_.read_exact(header, sizeof(header), err)) {
        return std::nullopt;
    }
    if (std::memcmp(header, kRpPairingMagic.data(), kRpPairingMagic.size()) != 0) {
        err = MAA_IOS_TR("Frame header is not RPPairing; check port and stream alignment");
        return std::nullopt;
    }
    const size_t len = (static_cast<size_t>(header[kRpPairingMagic.size()]) << 8) | static_cast<size_t>(header[kRpPairingMagic.size() + 1]);
    Bytes body(len);
    if (!io_.read_exact(body.data(), len, err)) {
        return std::nullopt;
    }
    return json::parse(
        std::string_view(reinterpret_cast<const char*>(body.data()), body.size()), // NOLINT
        &err);
}

bool Rppairing::send_envelope(const json::Value& message, std::string& err)
{
    return carrier_.write_envelope(message, err);
}

bool Rppairing::send_plain(const json::Value& inner, std::string& err)
{
    json::Value slot = j_obj({ { "_0", inner } });
    json::Value message = j_obj({ { "plain", std::move(slot) } });
    const json::Value envelope = j_obj(
        { { "message", std::move(message) },
          { "originatedBy", j_str("host") },
          { "sequenceNumber", j_int(static_cast<int64_t>(sequence_)) } });
    // sequenceNumber 在这里推进，而加密请求不推进——见类注释那条规矩的由来。
    if (!send_envelope(envelope, err)) {
        return false;
    }
    ++sequence_;
    return true;
}

std::optional<json::Value> Rppairing::receive(std::string& err)
{
    const std::optional<json::Value> envelope = carrier_.read_envelope(err);
    if (!envelope) {
        return std::nullopt;
    }

    const json::Value* message = json::find(*envelope, "message");
    if (message == nullptr) {
        err = MAA_IOS_TR("Envelope missing message field");
        return std::nullopt;
    }
    if (const json::Value* plain = json::find(*message, "plain")) {
        const json::Value* inner = json::find(*plain, "_0");
        if (inner == nullptr) {
            err = MAA_IOS_TR("plain missing _0");
            return std::nullopt;
        }
        return std::optional<json::Value>(std::in_place, *inner);
    }
    const json::Value* encrypted = json::find(*message, "streamEncrypted");
    if (encrypted == nullptr) {
        err = MAA_IOS_TR("Envelope is neither plain nor streamEncrypted");
        return std::nullopt;
    }
    const json::Value* payload = json::find(*encrypted, "_0");
    if (payload == nullptr || !payload->is_string()) {
        err = MAA_IOS_TR("streamEncrypted._0 is not a string");
        return std::nullopt;
    }
    if (server_main_.size() != 32 || last_nonce_.size() != 12) {
        err = MAA_IOS_TR("Encrypted frame received before main key and nonce were ready");
        return std::nullopt;
    }
    const std::optional<Bytes> sealed = b64_decode(json::as_string_or(*payload), err);
    if (!sealed) {
        return std::nullopt;
    }
    const std::optional<Bytes> plain_bytes = chacha_open(sv(server_main_), last_nonce_, *sealed, err);
    if (!plain_bytes) {
        return std::nullopt;
    }
    const std::optional<json::Value> decrypted = json::parse(
        std::string_view(reinterpret_cast<const char*>(plain_bytes->data()), plain_bytes->size()), // NOLINT
        &err);
    if (!decrypted) {
        return std::nullopt;
    }
    const json::Value* response = json::find(*decrypted, "response");
    if (response == nullptr) {
        err = MAA_IOS_TR("Decrypted message is not a response");
        return std::nullopt;
    }
    const json::Value* body_of_response = json::find(*response, "_1");
    if (body_of_response == nullptr) {
        err = MAA_IOS_TR("response missing _1");
        return std::nullopt;
    }
    // 设备把错误也塞在加密回复里：不挑出来的话，调用方会拿一个没有期待字段的对象
    // 去报"缺字段"，把一个本来很直白的拒绝变成看不懂的话。
    if (const json::Value* extended = json::find(*body_of_response, "errorExtended")) {
        const json::Value* info = json::find(*extended, "_0");
        const json::Value* user = info != nullptr ? json::find(*info, "userInfo") : nullptr;
        const json::Value* why = user != nullptr ? json::find(*user, "NSLocalizedDescription") : nullptr;
        err = MAA_IOS_TR("Device rejected request: ")
              + (why != nullptr ? json::as_string_or(*why) : std::string(MAA_IOS_TR("(no description)")));
        return std::nullopt;
    }
    return std::optional<json::Value>(std::in_place, *body_of_response);
}

std::optional<json::Value> Rppairing::plain_roundtrip(const json::Value& inner, std::string& err)
{
    if (!send_plain(inner, err)) {
        return std::nullopt;
    }
    return receive(err);
}

bool send_pairing_data(
    Rppairing& channel,
    const Bytes& tlv,
    std::string_view kind,
    bool start_new_session,
    std::string& err,
    std::string_view sending_host)
{
    std::vector<std::pair<std::string, json::Value>> fields = { { "data", j_str(b64_encode(tlv)) },
                                                                { "kind", j_str(kind) },
                                                                { "startNewSession", j_bool(start_new_session) } };
    if (!sending_host.empty()) {
        fields.emplace_back("sendingHost", j_str(sending_host));
    }
    const json::Value payload = j_obj(std::move(fields));
    // 外面那层 `_0` 是 event 的联合体包装，不是 pairingData 的：少了它设备直接关连接
    // （真机现场：握手答得好好的，第一条 pairingData 发出去就没有然后了）。
    json::Value wrapper = j_obj({ { "pairingData", j_obj({ { "_0", payload } }) } });
    const json::Value inner = j_obj({ { "event", j_obj({ { "_0", std::move(wrapper) } }) } });
    return channel.send_plain(inner, err);
}

std::optional<Bytes> pairing_data_roundtrip(
    Rppairing& channel,
    const Bytes& tlv,
    std::string_view kind,
    bool start_new_session,
    std::string& err,
    std::string_view sending_host,
    const ProgressFn& progress)
{
    if (!send_pairing_data(channel, tlv, kind, start_new_session, err, sending_host)) {
        return std::nullopt;
    }

    bool consent_pending = true; // 第一帧可能是 awaitingUserConsent，之后就不是了
    while (true) {
        const std::optional<json::Value> reply = channel.receive(err);
        if (!reply) {
            return std::nullopt;
        }
        const json::Value* event = json::find(*reply, "event");
        if (event == nullptr) {
            err = MAA_IOS_TR("Expected event during pairing");
            return std::nullopt;
        }
        const json::Value* zero = json::find(*event, "_0");
        if (zero == nullptr) {
            err = MAA_IOS_TR("event missing _0");
            return std::nullopt;
        }
        if (const json::Value* rejected = json::find(*zero, "pairingRejectedWithError")) {
            const json::Value* wrapped = json::find(*rejected, "wrappedError");
            const json::Value* user = wrapped != nullptr ? json::find(*wrapped, "userInfo") : nullptr;
            const json::Value* why = user != nullptr ? json::find(*user, "NSLocalizedDescription") : nullptr;
            err = MAA_IOS_TR("Device rejected request: ")
                  + (why != nullptr ? json::as_string_or(*why) : std::string(MAA_IOS_TR("(no description)")));
            return std::nullopt;
        }
        if (json::find(*zero, "awaitingUserConsent") != nullptr) {
            if (!consent_pending) {
                err = MAA_IOS_TR("Device requested user consent twice consecutively");
                return std::nullopt;
            }
            consent_pending = false;
            if (progress) {
                progress(MAA_IOS_TR("Device is waiting for Trust confirmation on its screen"));
            }
            continue;
        }
        const json::Value* data = json::find(*zero, "pairingData");
        const json::Value* inner_data = data != nullptr ? json::find(*data, "_0") : nullptr;
        const json::Value* bytes = inner_data != nullptr ? json::find(*inner_data, "data") : nullptr;
        if (bytes == nullptr || !bytes->is_string()) {
            err = MAA_IOS_TR("event missing pairingData._0.data; actual fields: ");
            if (zero->is_object()) {
                for (const auto& kv : zero->as_object()) {
                    err += " " + kv.first;
                }
            }
            return std::nullopt;
        }
        return b64_decode(json::as_string_or(*bytes), err);
    }
}

std::optional<json::Value> Rppairing::encrypted_roundtrip(const json::Value& request, std::string& err)
{
    if (client_main_.size() != 32) {
        err = MAA_IOS_TR("Main key not installed; cannot send encrypted frame");
        return std::nullopt;
    }
    // nonce = u64 小端计数 + 4 个零字节。计数在**往返成功之后**才推进。
    uint8_t nonce[12] = { 0 };
    const uint64_t counter = encrypted_sequence_;
    for (size_t i = 0; i < 8; ++i) {
        nonce[i] = static_cast<uint8_t>((counter >> (8 * i)) & 0xFF);
    }
    last_nonce_.assign(reinterpret_cast<const char*>(nonce), sizeof(nonce)); // NOLINT
    const std::string plain_text = json::write(request);
    const Bytes plain(plain_text.begin(), plain_text.end());
    const std::optional<Bytes> sealed = chacha_seal(sv(client_main_), last_nonce_, plain, err);
    if (!sealed) {
        return std::nullopt;
    }
    json::Value slot = j_obj({ { "_0", j_str(b64_encode(*sealed)) } });
    json::Value message = j_obj({ { "streamEncrypted", std::move(slot) } });
    const json::Value envelope = j_obj(
        { { "message", std::move(message) },
          { "originatedBy", j_str("host") },
          { "sequenceNumber", j_int(static_cast<int64_t>(sequence_)) } });
    if (!send_envelope(envelope, err)) {
        return std::nullopt;
    }
    std::optional<json::Value> response = receive(err);
    if (!response) {
        return std::nullopt;
    }
    ++encrypted_sequence_;
    return response;
}

} // namespace maa::ios::wifi
