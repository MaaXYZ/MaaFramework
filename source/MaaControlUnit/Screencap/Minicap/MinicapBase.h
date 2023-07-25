#pragma once

#include "UnitBase.h"

#include "../ScreencapHelper.h"
#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapBase : public ScreencapBase
{
public:
    MinicapBase()
    {
        children_.emplace_back(binary_);
        children_.emplace_back(library_);
    }
    virtual ~MinicapBase() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int w, int h) override;
    virtual void deinit() override {}
    virtual std::optional<cv::Mat> screencap() override = 0;

protected:
    ScreencapHelper screencap_helper_;

    std::shared_ptr<InvokeApp> binary_ = std::make_shared<InvokeApp>();
    std::shared_ptr<InvokeApp> library_ = std::make_shared<InvokeApp>();

private:
    std::string root_;
    std::vector<std::string> arch_list_;
    std::vector<int> sdk_list_;
};

MAA_CTRL_UNIT_NS_END
