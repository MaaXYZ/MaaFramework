#pragma once

#include <functional>
#include <optional>
#include <string>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapHelper
{
public:
    void set_wh(int w, int h);
    int get_w() const { return width_; }
    int get_h() const { return height_; }

    std::optional<cv::Mat> process_data(std::string& buffer,
                                        std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder);
    std::optional<cv::Mat> decode_raw(const std::string& buffer);
    std::optional<cv::Mat> decode_gzip(const std::string& buffer);
    std::optional<cv::Mat> decode_png(const std::string& buffer);
    std::optional<cv::Mat> decode_jpg(const std::string& buffer);
    std::optional<cv::Mat> decode_jpg_with_minicap_header(const std::string& buffer);
    static bool clean_cr(std::string& buffer);

protected:
    int width_ = 0;
    int height_ = 0;

private:
    enum class EndOfLine
    {
        UnknownYet,
        CRLF,
        LF,
        CR
    } end_of_line_ = EndOfLine::UnknownYet;
};

MAA_CTRL_UNIT_NS_END
