#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapFastestWay : public ScreencapBase
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
        MumuExternalRendererIpc,
    };

public:
    ScreencapFastestWay(const std::filesystem::path& minicap_path, bool lossless);
    virtual ~ScreencapFastestWay() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int swidth, int sheight) override;
    virtual void deinit() override;
    virtual bool set_wh(int swidth, int sheight) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    bool speed_test();

    std::unordered_map<Method, std::shared_ptr<ScreencapBase>> units_;
    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, ScreencapFastestWay::Method m);

MAA_CTRL_UNIT_NS_END
