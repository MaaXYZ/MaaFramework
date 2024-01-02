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
    std::string sync_context_to_arg(MaaSyncContextHandle sync_context);
    MaaSyncContextHandle arg_to_sync_context(const std::string& arg) const;

    std::string image_to_arg(const cv::Mat& image);

private:
    std::map<std::string, MaaSyncContextHandle> sync_contexts_;
    std::vector<std::filesystem::path> images_;
};

MAA_TOOLKIT_NS_END
