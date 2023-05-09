#include "GlobalOption.h"

#include "Common/MaaOptions.h"
#include "Utils/Platform.hpp"

MAA_NS_BEGIN

bool GlabalOption::set_option(std::string_view key, std::string_view value)
{
    using namespace option::global;

    if (key == LogDir) {
        log_dir_ = path(value);
    }
    else {
        return false;
    }

    return true;
}

MAA_NS_END
