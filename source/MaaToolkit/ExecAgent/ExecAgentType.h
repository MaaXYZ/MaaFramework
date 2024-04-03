#pragma once

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

#include "Conf/Conf.h"

MAA_TOOLKIT_NS_BEGIN

struct ExecData
{
    std::string name;
    std::filesystem::path exec_path;
    std::vector<std::string> exec_args;
};

MAA_TOOLKIT_NS_END
