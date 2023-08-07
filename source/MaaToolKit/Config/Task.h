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
    inline static const std::string kNameKey = "name";
    inline static const std::string kDescriptionKey = "description";
    inline static const std::string kEntryKey = "entry";
    inline static const std::string kParamKey = "param";
    inline static const std::string kEnabledKey = "enabled";

public:
    virtual ~Task() override = default;

public: // from MaaToolKitTaskAPI
    virtual std::string_view get_name() const override { return name_; }
    virtual void set_name(std::string_view new_name) override;
    virtual std::string_view get_description() const override { return description_; }
    virtual void set_description(std::string_view new_description) override;
    virtual std::string_view get_entry() const override { return entry_; }
    virtual void set_entry(std::string_view new_entry) override;
    virtual std::string_view get_param() const override { return param_; }
    virtual void set_param(std::string_view new_param) override;
    virtual bool get_enabled() const override { return enabled_; }
    virtual void set_enabled(bool new_enabled) override;

    virtual MaaStatus status() const override;

public:
    json::value to_json() const;
    bool from_json(const json::value& json);

    friend std::ostream& operator<<(std::ostream& os, const Task& task);

private:
    std::string name_;
    std::string description_;
    std::string entry_;
    std::string param_;
    bool enabled_ = true;
};

MAA_TOOLKIT_CONFIG_NS_END
