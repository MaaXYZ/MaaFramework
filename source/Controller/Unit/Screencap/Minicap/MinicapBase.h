#pragma once

#include "../ScreencapBase.h"

#include "../../Utils/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapBase : public ScreencapBase
{
public:
    MinicapBase()
    {
        children_.emplace_back(binary_);
        children_.emplace_back(library_);
    }

    bool parse(const json::value& config) override;

    bool init(int w, int h) override;

protected:
    std::shared_ptr<InvokeApp> binary_ = std::make_shared<InvokeApp>();
    std::shared_ptr<InvokeApp> library_ = std::make_shared<InvokeApp>();

private:
    std::vector<std::string> arch_list_;
    std::vector<int> sdk_list_;
};

MAA_CTRL_UNIT_NS_END
