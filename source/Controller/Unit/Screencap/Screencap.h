#pragma once

#include "Encode.h"
#include "EncodeToFile.h"
#include "RawByNetcat.h"
#include "RawWithGzip.h"

MAA_CTRL_UNIT_NS_BEGIN

class Screencap : public UnitBase
{
public:
    enum class Method
    {
        UnknownYet,
        RawByNetcat,
        RawWithGzip,
        Encode,
        EncodeToFileAndPull,
    };

public:
    bool parse(const json::value& config);

    bool init(int w, int h, const std::string& force_temp = "");
    void deinit();

    std::optional<cv::Mat> screencap();

private:
    bool speed_test();

    ScreencapRawByNetcat screencap_raw_by_netcat_uint_;
    ScreencapRawWithGzip screencap_raw_with_gzip_unit_;
    ScreencapEncode screencap_encode_unit_;
    ScreencapEncodeToFileAndPull screencap_encode_to_file_unit_;

    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, Screencap::Method m);

MAA_CTRL_UNIT_NS_END
