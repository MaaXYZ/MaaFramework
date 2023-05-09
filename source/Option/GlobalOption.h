#pragma once

#include <filesystem>
#include <string_view>

#include "Common/MaaConf.h"
#include "Utils/SingletonHolder.hpp"

MAA_NS_BEGIN

class GlabalOption : public SingletonHolder<GlabalOption>
{
public:
    friend class SingletonHolder<GlabalOption>;

public:
    virtual ~GlabalOption() = default;

    bool set_option(std::string_view key, std::string_view value);

    const std::filesystem::path& log_dir() const { return log_dir_; }

protected:
    GlabalOption() = default;

    std::filesystem::path log_dir_;
};

MAA_NS_END
