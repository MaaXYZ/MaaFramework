#pragma once

#include "ControlUnit/DebuggingControlUnitAPI.h"

#include <filesystem>

MAA_DBG_CTRL_UNIT_NS_BEGIN

class ReadIndex : public ScreencapAPI
{
public:
    ReadIndex(std::filesystem::path indexfile);
    virtual ~ReadIndex() override = default;

public: // from ScreencapAPI
    virtual bool init(int swidth, int sheight) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    int swidth_ = 0;
    int sheight_ = 0;

    std::filesystem::path indexfile_;
    std::vector<std::filesystem::path> filepaths_;
    size_t index_ = 0;
};

MAA_DBG_CTRL_UNIT_NS_END
