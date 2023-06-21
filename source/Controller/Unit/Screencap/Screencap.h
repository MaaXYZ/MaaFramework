#pragma once

#include "Encode.h"
#include "EncodeToFile.h"
#include "Minicap/MinicapDirect.h"
#include "Minicap/MinicapStream.h"
#include "RawByNetcat.h"
#include "RawWithGzip.h"

MAA_CTRL_UNIT_NS_BEGIN

class Screencap : public ScreencapBase
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

    bool parse(const json::value& config) override;

    bool init(int w, int h) override;
    void deinit() override;

    std::optional<cv::Mat> screencap() override;

#ifdef MAA_DEBUG
public:
    const std::vector<std::shared_ptr<ScreencapBase>>& get_units() { return units_; }
#endif

private:
    bool speed_test();

    std::vector<std::shared_ptr<ScreencapBase>> units_;

    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, Screencap::Method m);

MAA_CTRL_UNIT_NS_END
