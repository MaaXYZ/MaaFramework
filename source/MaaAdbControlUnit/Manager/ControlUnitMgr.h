#pragma once

#include <filesystem>

#include "Base/UnitBase.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "General/Activity.h"
#include "General/Connection.h"
#include "General/DeviceInfo.h"
#include "General/DeviceList.h"
#include "Utils/Dispatcher.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ControlUnitMgr
    : public ControlUnitAPI
    , public Dispatcher<ControlUnitSink>
{
public:
    ControlUnitMgr(
        std::filesystem::path adb_path,
        std::string adb_serial,
        std::shared_ptr<ScreencapBase> screencap_unit,
        std::shared_ptr<InputBase> touch_unit);
    virtual ~ControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual bool find_device(/*out*/ std::vector<std::string>& devices) override;

    virtual bool connect() override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

public:
    bool parse(const json::value& config);
    void set_replacement(const UnitBase::Replacement& replacement);

private:
    bool _screencap(/*out*/ cv::Mat& image);
    void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur);
    void on_app_started(const std::string& intent);
    void on_app_stopped(const std::string& intent);

private:
    std::filesystem::path adb_path_;
    std::string adb_serial_;

    DeviceList device_list_;
    Connection connection_;
    DeviceInfo device_info_;
    Activity activity_;

    std::shared_ptr<InputBase> input_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;

    bool screencap_available_ = false;
    std::pair<int, int> image_resolution_;
};

MAA_CTRL_UNIT_NS_END
