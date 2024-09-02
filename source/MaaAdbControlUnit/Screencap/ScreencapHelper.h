#pragma once

#include <functional>
#include <optional>
#include <string>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapHelper
{
public:
    std::optional<cv::Mat> process_data(std::string& buffer, std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder);

    static std::optional<cv::Mat> decode_raw(const std::string& buffer);
    static std::optional<cv::Mat> decode_gzip(const std::string& buffer);
    static std::optional<cv::Mat> decode_png(const std::string& buffer);
    static std::optional<cv::Mat> trunc_decode_jpg(const std::string& buffer);
    static std::optional<cv::Mat> decode_jpg(const std::string& buffer);
    static std::optional<cv::Mat> decode(const std::string& buffer);

private:
    static bool clean_cr(std::string& buffer);
    static bool check_head_tail(std::string_view input, std::string_view head, std::string_view tail);

    enum class EndOfLine
    {
        UnknownYet,
        CRLF,
        LF,
        CR
    } end_of_line_ = EndOfLine::UnknownYet;
};

MAA_CTRL_UNIT_NS_END
