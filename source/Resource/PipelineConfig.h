#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include <filesystem>

MAA_RES_NS_BEGIN

class PipelineConfig : public NonCopyable
{
public:
    bool load(const std::filesystem::path& path, bool is_base);
};

MAA_RES_NS_END
