#pragma once

#include <filesystem>
#include <map>

#include "Conf/Conf.h"
#include "ExecAgentType.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_TOOLKIT_NS_BEGIN

class ExecArgConverter
{
public:
    ~ExecArgConverter();

public:
    std::string context_to_arg(MaaContext* context);
    MaaContext* arg_to_context(const std::string& arg) const;

    std::string image_to_arg(const cv::Mat& image);

private:
    std::map<std::string, MaaContext*> contexts_;
    std::vector<std::filesystem::path> images_;
};

MAA_TOOLKIT_NS_END
