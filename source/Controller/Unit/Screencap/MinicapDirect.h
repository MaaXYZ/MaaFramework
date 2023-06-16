#pragma once

#include "ScreencapBase.h"

#include "../Utils/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapDirect : public ScreencapBase
{
public:
    MinicapDirect() : binary_(new InvokeApp), library_(new InvokeApp)
    {
        children_.push_back(binary_.get());
        children_.push_back(library_.get());
    }

    bool parse(const json::value& config);

    bool init(int w, int h, std::function<std::string(const std::string&)> path_of_bin,
              std::function<std::string(const std::string&, int)> path_of_lib, const std::string& force_temp = "");
    void deinit() {}

    std::optional<cv::Mat> screencap();

private:
    std::shared_ptr<InvokeApp> binary_, library_;
};

MAA_CTRL_UNIT_NS_END
