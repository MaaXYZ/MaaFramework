#include "AnnexB.h"

#include <algorithm>

namespace maa::ios
{
namespace
{

constexpr uint8_t nal_type_of(const std::vector<uint8_t>& nal)
{
    return nal.size() >= 2 ? static_cast<uint8_t>((nal[0] >> 1) & 0x3F) : 0xFF;
}

bool is_vcl(uint8_t type)
{
    return type <= 31;
}

bool is_irap(uint8_t type)
{
    return type >= 16 && type <= 21;
}

bool is_param_set(uint8_t type)
{
    return type == 32 || type == 33 || type == 34;
}

bool is_prefix(uint8_t type)
{
    return is_param_set(type) || type == 35 || type == static_cast<uint8_t>(NalType::SeiPrefix);
}

/// slice segment header 的第一个 bit 就是 first_slice_segment_in_pic_flag，
/// 紧跟在 2 字节 NAL header 之后。
bool starts_new_picture(const Nal& nal)
{
    return nal.size() > 2 && (nal[2] & 0x80) != 0;
}

} // namespace

bool AnnexBParser::feed(const uint8_t* data, std::size_t len)
{
    if (len == 0) {
        return true;
    }
    if (!data || input_mode_ == InputMode::Nals) {
        return false;
    }
    input_mode_ = InputMode::Bytes;
    pending_.insert(pending_.end(), data, data + len);

    std::size_t i = scan_from_;
    while (i + 3 <= pending_.size()) {
        if (pending_[i] == 0 && pending_[i + 1] == 0 && pending_[i + 2] == 1) {
            std::size_t end = i;
            if (end > nal_begin_ && pending_[end - 1] == 0) {
                --end; // 四字节起始码，前一个 0 也属于起始码
            }
            emit_range(end);
            nal_begin_ = i + 3;
            i = nal_begin_;
            continue;
        }
        ++i;
    }

    // 从最后两字节继续扫描，以识别跨 feed 边界的 00 00 01
    scan_from_ = std::max(nal_begin_, pending_.size() >= 2 ? pending_.size() - 2 : 0);

    if (nal_begin_ > 0) {
        pending_.erase(pending_.begin(), pending_.begin() + static_cast<long>(nal_begin_));
        scan_from_ -= nal_begin_;
        nal_begin_ = 0;
    }
    return true;
}

bool AnnexBParser::push_nal(Nal nal, std::optional<int64_t> sampling_timestamp, bool marker)
{
    if (input_mode_ == InputMode::Bytes) {
        return false;
    }
    input_mode_ = InputMode::Nals;
    if (!on_nal(std::move(nal), sampling_timestamp)) {
        discard_au();
        return false;
    }
    if (marker) {
        close_au();
    }
    return true;
}

void AnnexBParser::flush()
{
    emit_range(pending_.size());
    pending_.clear();
    scan_from_ = 0;
    nal_begin_ = 0;
    close_au();
    input_mode_ = InputMode::None;
}

void AnnexBParser::discard_pending()
{
    pending_.clear();
    scan_from_ = 0;
    nal_begin_ = 0;
    discard_au();
    input_mode_ = InputMode::None;
}

void AnnexBParser::emit_range(std::size_t end)
{
    if (end <= nal_begin_) {
        return;
    }
    std::vector<uint8_t> raw(pending_.begin() + static_cast<long>(nal_begin_), pending_.begin() + static_cast<long>(end));
    on_nal(std::move(raw), std::nullopt);
}

bool AnnexBParser::on_nal(Nal&& nal, std::optional<int64_t> sampling_timestamp)
{
    if (nal.size() < 3) {
        return false;
    }
    // NAL、解码样本和参数集缓存都保留原始 EPB；仅语法解析时另行转换为 RBSP。
    // 类型标记在两字节 NAL 头中，first_slice_segment_in_pic_flag 是其后第一个 bit，
    // 读取这两项不需要去除 EPB。
    const uint8_t type = nal_type_of(nal);

    if (is_vcl(type)) {
        // 新图像开始且当前 AU 已含 VCL 时提交前一 AU。
        // 尚未包含 VCL 的 VPS/SPS/PPS 前缀与本图像合并。
        const bool new_pic = starts_new_picture(nal);
        if (new_pic) {
            if (au_has_vcl_) {
                close_au();
            }
        }
        else if (!au_has_vcl_) {
            return false; // 参数前缀不能替代图像起始 slice
        }
        else if (au_sampling_timestamp_ != sampling_timestamp) {
            // 不能把时间不一致的 slice 拼成完整图像，也不能用后一条的时间补猜前一条。
            return false;
        }
        if (!au_has_vcl_) {
            au_sampling_timestamp_ = sampling_timestamp;
        }
        au_open_ = true;
        au_has_vcl_ = true;
        if (is_irap(type)) {
            au_keyframe_ = true;
        }
        cur_au_.push_back(std::move(nal));
        return true;
    }

    if (is_prefix(type)) {
        // 参数集、AUD 和 prefix SEI 属于下一 AU。当前 AU 已含 VCL 时先提交，
        // 尚无图像时继续累积前缀，避免把下一帧的 SEI 交给前一帧。
        if (au_has_vcl_) {
            close_au();
        }
        // 前一 AU 的回调可能使用缓存补齐未重复发送的参数集；提交完成后，
        // 才能让下一图像的参数覆盖缓存。
        if (type == static_cast<uint8_t>(NalType::Vps)) {
            vps_ = nal;
        }
        else if (type == static_cast<uint8_t>(NalType::Sps)) {
            sps_ = nal;
        }
        else if (type == static_cast<uint8_t>(NalType::Pps)) {
            pps_ = nal;
        }
        au_open_ = true;
        cur_au_.push_back(std::move(nal));
        return true;
    }

    if (au_open_) {
        cur_au_.push_back(std::move(nal));
    }
    return true;
}

void AnnexBParser::close_au()
{
    if (au_has_vcl_ && !cur_au_.empty()) {
        AccessUnit au { std::move(cur_au_), au_keyframe_, au_sampling_timestamp_ };
        discard_au();
        on_au_(std::move(au));
        return;
    }
    discard_au();
}

void AnnexBParser::discard_au()
{
    cur_au_.clear();
    au_open_ = false;
    au_has_vcl_ = false;
    au_keyframe_ = false;
    au_sampling_timestamp_.reset();
}

} // namespace maa::ios
