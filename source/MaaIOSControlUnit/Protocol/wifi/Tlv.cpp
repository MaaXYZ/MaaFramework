#include "wifi/Tlv.h"

#include <algorithm>

namespace maa::ios::wifi
{

Bytes tlv_build(const std::vector<std::pair<TlvType, Bytes>>& items)
{
    Bytes out;
    for (const auto& [type, value] : items) {
        size_t off = 0;
        do {
            const size_t chunk = std::min<size_t>(255, value.size() - off);
            out.push_back(static_cast<uint8_t>(type));
            out.push_back(static_cast<uint8_t>(chunk));
            out.insert(out.end(), value.begin() + off, value.begin() + off + chunk);
            off += chunk;
        } while (off < value.size());
        // 空值也要占一条（len=0）：拆分循环在上面那条语句里已经覆盖了——
        // value.size()==0 时 off=0、chunk=0，仍写一条长度为 0 的 TLV。
    }
    return out;
}

std::map<uint8_t, Bytes> tlv_parse(const Bytes& data, std::string& err, bool* truncated)
{
    std::map<uint8_t, Bytes> fields;
    size_t i = 0;
    while (i < data.size()) {
        if (i + 2 > data.size()) {
            err = "TLV header truncated";
            if (truncated != nullptr) {
                *truncated = true;
            }
            return fields;
        }
        const uint8_t type = data[i];
        const size_t len = data[i + 1];
        if (i + 2 + len > data.size()) {
            err = "TLV length exceeds buffer";
            if (truncated != nullptr) {
                *truncated = true;
            }
            return fields;
        }
        // 同类型多次出现要拼接（见 `tlv_build` 的拆分规矩），不是"后者覆盖前者"。
        fields[type].insert(fields[type].end(), data.begin() + i + 2, data.begin() + i + 2 + len);
        i += 2 + len;
    }
    if (truncated != nullptr) {
        *truncated = false;
    }
    return fields;
}

const Bytes* tlv_get(const std::map<uint8_t, Bytes>& fields, TlvType type)
{
    const auto it = fields.find(static_cast<uint8_t>(type));
    return it == fields.end() ? nullptr : &it->second;
}

} // namespace maa::ios::wifi
