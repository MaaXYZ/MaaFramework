#include "ScreencapHelper.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4068)
#endif
#include <gzip/decompress.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat>
    ScreencapHelper::process_data(std::string& buffer, std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder)
{
    bool tried_clean = false;

#ifdef _WIN32
    if (end_of_line_ == EndOfLine::UnknownYet) {
        auto saved = buffer;
        if (clean_cr(buffer)) {
            auto res = decoder(buffer);
            if (res) {
                LogInfo << "end_of_line is CRLF";
                end_of_line_ = EndOfLine::CRLF;
                return res;
            }
            else {
                saved.swap(buffer);
            }
        }
        tried_clean = true;
    }
#endif

    if (end_of_line_ == EndOfLine::CRLF) {
        tried_clean = true;
        if (!clean_cr(buffer)) {
            LogInfo << "end_of_line is set to CRLF but no `\\r\\n` found, set it to LF";
            end_of_line_ = EndOfLine::LF;
        }
    }

    auto res = decoder(buffer);

    if (res) {
        if (end_of_line_ == EndOfLine::UnknownYet) {
            LogInfo << "end_of_line is LF";
            end_of_line_ = EndOfLine::LF;
        }
        return res;
    }

    LogInfo << "data is not empty, but image is empty";
    if (tried_clean) {
        LogError << "skip retry decoding and decode failed!";
        return std::nullopt;
    }

    LogInfo << "try to cvt lf";
    if (!clean_cr(buffer)) {
        LogError << "no `\\r\\n` found, skip retry decode";
        return std::nullopt;
    }

    res = decoder(buffer);

    if (!res) {
        LogError << "convert lf and retry decode failed!";
        return std::nullopt;
    }

    if (end_of_line_ == EndOfLine::UnknownYet) {
        LogInfo << "end_of_line is CRLF";
    }
    else {
        LogInfo << "end_of_line is changed to CRLF";
    }
    end_of_line_ = EndOfLine::CRLF;

    return res;
}

std::optional<cv::Mat> ScreencapHelper::decode_raw(const std::string& buffer)
{
    if (buffer.size() < 8) {
        return std::nullopt;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer.data());
    uint32_t im_width = 0, im_height = 0;
    memcpy(&im_width, data, 4);
    memcpy(&im_height, data + 4, 4);

    size_t size = 4ull * im_width * im_height;

    if (buffer.size() < size) {
        return std::nullopt;
    }

    size_t header_size = buffer.size() - size;
    const uint8_t* im_data = data + header_size;

    cv::Mat temp(im_height, im_width, CV_8UC4, const_cast<uint8_t*>(im_data));
    if (temp.empty()) {
        return std::nullopt;
    }

    const auto& br = *(temp.end<cv::Vec4b>() - 1);
    if (br[3] != 255) { // only check alpha
        return std::nullopt;
    }
    cv::cvtColor(temp, temp, cv::COLOR_RGBA2BGR);
    return temp.clone(); // temp只是引用data, 需要clone确保数据拥有所有权
}

std::optional<cv::Mat> ScreencapHelper::decode_gzip(const std::string& buffer)
{
    auto buf = gzip::decompress(buffer.c_str(), buffer.size());
    return decode_raw(buf);
}

std::optional<cv::Mat> ScreencapHelper::decode_png(const std::string& buffer)
{
    if (!check_head_tail(buffer, "\x89\x50\x4E\x47", "\xAE\x42\x60\x82")) {
        return std::nullopt;
    }
    return decode(buffer);
}

std::optional<cv::Mat> ScreencapHelper::trunc_decode_jpg(const std::string& buffer)
{
    auto pos = buffer.find("\xFF\xD8\xFF");
    auto truncbuf = buffer.substr(pos);
    return decode_jpg(truncbuf);
}

std::optional<cv::Mat> ScreencapHelper::decode_jpg(const std::string& buffer)
{
    if (!check_head_tail(buffer, "\xFF\xD8\xFF", "\xFF\xD9")) {
        return std::nullopt;
    }
    return decode(buffer);
}

std::optional<cv::Mat> ScreencapHelper::decode(const std::string& buffer)
{
    cv::Mat img = cv::imdecode({ buffer.data(), static_cast<int>(buffer.size()) }, cv::IMREAD_COLOR);
    return img.empty() ? std::nullopt : std::make_optional(img);
}

bool ScreencapHelper::clean_cr(std::string& buffer)
{
    if (buffer.size() < 2) {
        return false;
    }

    auto check = [](std::string::iterator it) {
        return *it == '\r' && *(it + 1) == '\n';
    };

    auto scan = buffer.end();
    for (auto it = buffer.begin(); it != buffer.end() - 1; ++it) {
        if (check(it)) {
            scan = it;
            break;
        }
    }
    if (scan == buffer.end()) {
        return false;
    }

    auto last = buffer.end() - 1;
    auto ptr = scan;
    while (++scan != last) {
        if (!check(scan)) {
            *ptr = *scan;
            ++ptr;
        }
    }
    *ptr = *last;
    ++ptr;
    buffer.erase(ptr, buffer.end());
    return true;
}

bool ScreencapHelper::check_head_tail(std::string_view input, std::string_view head, std::string_view tail)
{
    if (input.size() < head.size() || input.size() < tail.size()) {
        LogError << "input too short" << VAR(input) << VAR(head) << VAR(tail);
        return false;
    }

    if (input.substr(0, head.size()) != head || input.substr(input.size() - tail.size(), tail.size()) != tail) {
        LogError << "head or tail mismatch" << VAR(input) << VAR(head) << VAR(tail);
        return false;
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
