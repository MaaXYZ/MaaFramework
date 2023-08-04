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
    virtual std::string_view get_name() const override { return name_; }
    virtual void set_name(std::string_view new_name) override;
    virtual std::string_view get_description() const override { return description_; }
    virtual void set_description(std::string_view new_description) override;
    virtual std::string_view get_type() const override { return type_; }
    virtual void set_type(std::string_view new_type) override;
    virtual std::string_view get_param() const override { return param_; }
    virtual void set_param(std::string_view new_param) override;
    virtual bool get_enabled() const override { return enabled_; }
    virtual void set_enabled(bool new_enabled) override;

    virtual MaaStatus status() const override;

private:
    std::string name_;
    std::string description_;
    std::string type_;
    std::string param_;
    bool enabled_ = true;
};

std::ostream& operator<<(std::ostream& os, const Task& task);

MAA_TOOLKIT_CONFIG_NS_END
