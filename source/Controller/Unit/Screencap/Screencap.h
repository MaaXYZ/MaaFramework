#pragma once

#include "Encode.h"
#include "EncodeToFile.h"
#include "Minicap/MinicapDirect.h"
#include "Minicap/MinicapStream.h"
#include "RawByNetcat.h"
#include "RawWithGzip.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI Screencap : public ScreencapBase
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
    virtual ~Screencap() override = default;

    virtual bool parse(const json::value& config) override;

    virtual bool init(int w, int h) override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;

    std::shared_ptr<ScreencapBase>& get_unit(Method method) { return units_[method]; }

private:
    bool speed_test();

    std::map<Method, std::shared_ptr<ScreencapBase>> units_;

    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, Screencap::Method m);

MAA_CTRL_UNIT_NS_END
