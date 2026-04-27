#include "MinitouchInput.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

MinitouchInput::~MinitouchInput()
{
    remove_binary();
}

bool MinitouchInput::parse(const json::value& config)
{
    return parse_minitouch_config(config);
}

bool MinitouchInput::init()
{
    LogFunc;

    if (!push_minitouch()) {
        return false;
    }

    return invoke_and_read_info();
}

void MinitouchInput::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    std::ignore = pre;
    std::ignore = cur;

    invoke_and_read_info();
}

bool MinitouchInput::invoke_and_read_info()
{
    // https://github.com/openstf/minitouch#running
    static const std::string kMinitouchUseStdin = "-i";
    pipe_ios_ = invoke_app_->invoke_bin(kMinitouchUseStdin);
    if (!pipe_ios_) {
        return false;
    }

    return read_info();
}

MAA_CTRL_UNIT_NS_END
