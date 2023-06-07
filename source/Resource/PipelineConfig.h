#pragma once

#include "Common/MaaConf.h"
#include "Base/NonCopyable.hpp"

#include <filesystem>

MAA_RES_NS_BEGIN

class PipelineConfig : public NonCopyable
{
public:
    bool load(const std::filesystem::path& path);
};

MAA_RES_NS_END
