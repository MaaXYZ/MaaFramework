#pragma once

#include <filesystem>

#include "../ScreencapHelper.h"
#include "Base/UnitBase.h"
#include "General/DeviceInfo.h"
#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapBase : public ScreencapBase
{
public:
    explicit MinicapBase(std::filesystem::path agent_path)
        : agent_path_(std::move(agent_path))
    {
        children_.emplace_back(binary_);
        children_.emplace_back(library_);
        children_.emplace_back(device_info_);
    }

    virtual ~MinicapBase() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override = 0;

protected:
    bool init_binary();
    void deinit_binary();

    std::shared_ptr<InvokeApp> binary_ = std::make_shared<InvokeApp>();
    std::shared_ptr<InvokeApp> library_ = std::make_shared<InvokeApp>();

    int display_width_ = 0;
    int display_height_ = 0;

private:
    std::filesystem::path agent_path_;
    std::vector<std::string> arch_list_;
    std::vector<int> sdk_list_;

    std::shared_ptr<DeviceInfo> device_info_ = std::make_shared<DeviceInfo>();
};

MAA_CTRL_UNIT_NS_END
