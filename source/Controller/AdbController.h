#pragma once
#include "ControllerMgr.h"

#include "Platform/PlatformIO.h"
#include "Unit/ControlUnit.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class AdbController : public ControllerMgr
{
public:
    struct ControlUnit
    {
        void set_io(const std::shared_ptr<PlatformIO>& io_ptr)
        {
            for (auto unit : units) {
                unit->set_io(io_ptr);
            }
        }

        bool parse(const json::value& config)
        {
            bool ret = true;
            for (auto unit : units) {
                ret = ret && unit->parse(config);
            }
            return ret;
        }

        std::shared_ptr<MAA_CTRL_UNIT_NS::Connection> connection;
        std::shared_ptr<MAA_CTRL_UNIT_NS::DeviceInfo> device_info;
        std::shared_ptr<MAA_CTRL_UNIT_NS::Activity> activity;
        std::shared_ptr<MAA_CTRL_UNIT_NS::TouchInputBase> touch_input;
        std::shared_ptr<MAA_CTRL_UNIT_NS::KeyInputBase> key_input;
        std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapBase> screencap;

        std::vector<std::shared_ptr<MAA_CTRL_UNIT_NS::UnitBase>> units;
    };

    static std::optional<ControlUnit> parse_config(const json::value& config,
                                                   std::shared_ptr<MAA_CTRL_UNIT_NS::TouchInputBase> touch,
                                                   std::shared_ptr<MAA_CTRL_UNIT_NS::KeyInputBase> key,
                                                   std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapBase> screencap)
    {
        ControlUnit result;

        result.connection = std::make_shared<MAA_CTRL_UNIT_NS::Connection>();
        result.units.push_back(result.connection);
        result.device_info = std::make_shared<MAA_CTRL_UNIT_NS::DeviceInfo>();
        result.units.push_back(result.device_info);
        result.activity = std::make_shared<MAA_CTRL_UNIT_NS::Activity>();
        result.units.push_back(result.activity);
        result.touch_input = touch;
        result.units.push_back(std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::UnitBase>(touch));
        result.key_input = key;
        if (key != touch) {
            result.units.push_back(std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::UnitBase>(key));
        }
        result.screencap = screencap;
        result.units.push_back(screencap);

        auto ret = result.parse(config);
        return ret ? std::make_optional(std::move(result)) : std::nullopt;
    }

public:
    AdbController(std::string adb_path, std::string address, ControlUnit config, MaaControllerCallback callback,
                  void* callback_arg);
    virtual ~AdbController() override;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;

private:
    std::string adb_path_;
    std::string address_;

    ControlUnit control_unit_;

private:
    std::shared_ptr<PlatformIO> platform_io_ = nullptr;
};

MAA_CTRL_NS_END
