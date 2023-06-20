#pragma once

#include "Encode.h"
#include "EncodeToFile.h"
#include "Minicap/MinicapDirect.h"
#include "Minicap/MinicapStream.h"
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
        MinicapDirect,
        MinicapStream,
    };

public:
    Screencap();

    bool parse(const json::value& config);

    bool init(int w, int h, const std::string& force_temp = "");
    void deinit();

    std::optional<cv::Mat> screencap();

#ifdef MAA_DEBUG
public:
    std::shared_ptr<ScreencapRawByNetcat> get_raw_by_netcat() { return raw_by_netcat_uint_; }
    std::shared_ptr<ScreencapRawWithGzip> get_raw_with_gzip() { return raw_with_gzip_unit_; }
    std::shared_ptr<ScreencapEncode> get_encode() { return encode_unit_; }
    std::shared_ptr<ScreencapEncodeToFileAndPull> get_encode_to_file() { return encode_to_file_unit_; }
    std::shared_ptr<MinicapDirect> get_minicap_direct() { return minicap_direct_unit_; }
    std::shared_ptr<MinicapStream> get_minicap_stream() { return minicap_stream_unit_; }
#endif

private:
    bool speed_test();

    std::shared_ptr<ScreencapRawByNetcat> raw_by_netcat_uint_ = std::make_shared<ScreencapRawByNetcat>();
    std::shared_ptr<ScreencapRawWithGzip> raw_with_gzip_unit_ = std::make_shared<ScreencapRawWithGzip>();
    std::shared_ptr<ScreencapEncode> encode_unit_ = std::make_shared<ScreencapEncode>();
    std::shared_ptr<ScreencapEncodeToFileAndPull> encode_to_file_unit_ =
        std::make_shared<ScreencapEncodeToFileAndPull>();
    std::shared_ptr<MinicapDirect> minicap_direct_unit_ = std::make_shared<MinicapDirect>();
    std::shared_ptr<MinicapStream> minicap_stream_unit_ = std::make_shared<MinicapStream>();

    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, Screencap::Method m);

MAA_CTRL_UNIT_NS_END
