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
        void set_io(const std::shared_ptr<PlatformIO>& io_ptr);
        bool parse(const json::value& config);

        std::shared_ptr<MAA_CTRL_UNIT_NS::Connection> connection = nullptr;
        std::shared_ptr<MAA_CTRL_UNIT_NS::DeviceInfo> device_info = nullptr;
        std::shared_ptr<MAA_CTRL_UNIT_NS::Activity> activity = nullptr;
        std::shared_ptr<MAA_CTRL_UNIT_NS::TouchInputBase> touch_input = nullptr;
        std::shared_ptr<MAA_CTRL_UNIT_NS::KeyInputBase> key_input = nullptr;
        std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapBase> screencap = nullptr;

        std::vector<std::shared_ptr<MAA_CTRL_UNIT_NS::UnitBase>> units;
    };

    static std::optional<ControlUnit> parse_config(const json::value& config,
                                                   std::shared_ptr<MAA_CTRL_UNIT_NS::TouchInputBase> touch,
                                                   std::shared_ptr<MAA_CTRL_UNIT_NS::KeyInputBase> key,
                                                   std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapBase> screencap);

public:
    AdbController(std::string adb_path, std::string address, ControlUnit config, MaaControllerCallback callback,
                  void* callback_arg);
    virtual ~AdbController() override;

    virtual std::string get_uuid() const override;

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
