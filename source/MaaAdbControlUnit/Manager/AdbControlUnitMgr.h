#pragma once

#include <filesystem>

#include "Base/UnitBase.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "General/Activity.h"
#include "General/AdbCommand.h"
#include "General/Connection.h"
#include "General/DeviceInfo.h"
#include "General/DeviceList.h"
#include "MaaUtils/Dispatcher.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class AdbControlUnitMgr
    : public AdbControlUnitAPI
    , public Dispatcher<ControlUnitSink>
{
public:
    AdbControlUnitMgr(
        std::filesystem::path adb_path,
        std::string adb_serial,
        MaaAdbScreencapMethod screencap_methods,
        MaaAdbInputMethod input_methods,
        json::object config,
        std::filesystem::path agent_path);

    virtual ~AdbControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual bool connect() override;
    virtual bool connected() const override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;
    virtual MaaControllerFeature get_features() const override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

public:
    virtual bool find_device(/*out*/ std::vector<std::string>& devices) override;
    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) override;

private:
    bool _screencap(/*out*/ cv::Mat& image);
    void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur);
    void on_app_started(const std::string& intent);
    void on_app_stopped(const std::string& intent);

private:
    const std::filesystem::path adb_path_;
    const std::string adb_serial_;
    const json::value config_;
    const MaaAdbScreencapMethod screencap_methods_ = MaaAdbScreencapMethod_None;
    const MaaAdbInputMethod input_methods_ = MaaAdbInputMethod_None;
    const std::filesystem::path agent_path_;
    const UnitBase::Replacement unit_replacement_;

    DeviceList device_list_;
    Connection connection_;
    DeviceInfo device_info_;
    Activity activity_;
    AdbCommand adb_command_;

    std::shared_ptr<InputBase> input_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;

    bool screencap_available_ = false;
    std::pair<int, int> image_resolution_;
};

MAA_CTRL_UNIT_NS_END
