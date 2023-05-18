#pragma once

#include "Common/MaaConf.h"

#include < filesystem>

MAA_RES_NS_BEGIN

class AbstractResource
{
public:
    virtual ~AbstractResource() = default;

public:
    virtual bool load(const std::filesystem::path& path) = 0;
};

MAA_NS_END
