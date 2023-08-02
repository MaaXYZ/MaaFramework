#pragma once

#include <ostream>
#include <string>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "ConfigAPI.h"

MAA_TOOLKIT_CONFIG_NS_BEGIN

class Task : public MaaToolKitTaskAPI
{
public:
    virtual ~Task() override = default;

public: // from MaaToolKitTaskAPI
    virtual MaaString get_name() const override { return name_.c_str(); }
    virtual void set_name(MaaString new_name) override;
    virtual MaaString get_description() const override { return description_.c_str(); }
    virtual void set_description(MaaString new_description) override;
    virtual MaaString get_type() const override { return type_.c_str(); }
    virtual void set_type(MaaString new_type) override;
    virtual MaaJsonString get_param() const override { return param_.c_str(); }
    virtual void set_param(MaaJsonString new_param);
    virtual MaaBool get_enabled() const override { return enabled_; }
    virtual void set_enabled(MaaBool new_enabled);

    virtual MaaStatus status() const override;

private:
    std::string name_;
    std::string description_;
    std::string type_;
    std::string param_;
    MaaBool enabled_ = true;
};

std::ostream& operator<<(std::ostream& os, const Task& task);

MAA_TOOLKIT_CONFIG_NS_END
