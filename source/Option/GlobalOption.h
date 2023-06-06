#pragma once

#include <filesystem>
#include <string_view>

#include "Base/SingletonHolder.hpp"
#include "Common/MaaConf.h"

MAA_NS_BEGIN

class GlabalOption : public SingletonHolder<GlabalOption>
{
public:
    friend class SingletonHolder<GlabalOption>;

public:
    virtual ~GlabalOption() = default;

    bool set_option(std::string_view key, std::string_view value);

protected:
    GlabalOption() = default;
};

MAA_NS_END
