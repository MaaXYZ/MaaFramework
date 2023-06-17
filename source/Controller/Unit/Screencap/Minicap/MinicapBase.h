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

    bool parse(const json::value& config);

    bool init(int w, int h, std::function<std::string(const std::string&)> path_of_bin,
              std::function<std::string(const std::string&, int)> path_of_lib, const std::string& force_temp = "");
    void deinit() {}

protected:
    std::shared_ptr<InvokeApp> binary_ = std::make_shared<InvokeApp>();
    std::shared_ptr<InvokeApp> library_ = std::make_shared<InvokeApp>();
};

MAA_CTRL_UNIT_NS_END
