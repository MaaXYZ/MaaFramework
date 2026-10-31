#pragma once

#include <string>
#include <utility>

#include "MaaControlUnit/ControlUnitAPI.h"

#include "Session/IOSSession.h"

MAA_CTRL_UNIT_NS_BEGIN

class IOSControlUnitMgr : public IOSControlUnitAPI
{
public:
    IOSControlUnitMgr(
        std::string udid,
        MaaIOSScreencapMethod screencap_methods,
        MaaIOSConnectionMethod connection_method,
        std::string config);
    ~IOSControlUnitMgr() override;

public: // from ControlUnitAPI
    bool connect() override;
    bool connected() const override;

    bool request_uuid(std::string& uuid) override;
    MaaControllerFeature get_features() const override;

    bool start_app(const std::string& intent) override;
    bool stop_app(const std::string& intent) override;

    bool screencap(cv::Mat& image) override;

    bool click(int x, int y) override;
    bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    bool touch_down(int contact, int x, int y, int pressure) override;
    bool touch_move(int contact, int x, int y, int pressure) override;
    bool touch_up(int contact) override;

    bool click_key(int key) override;
    bool input_text(const std::string& text) override;

    bool key_down(int key) override;
    bool key_up(int key) override;

    bool inactive() override;
    json::object get_info() const override;

private:
    /// 像素 -> 整块屏幕的 0..1。设备的触摸面本来就是归一化的，所以只需要知道
    /// 截图的尺寸；尺寸未知就先抓一张，而不是猜一个。
    bool normalize(int x, int y, double& fx, double& fy, std::string& err);
    /// 截图的原始尺寸。抓一张并解码，同时把尺寸缓存下来。
    bool refresh_display_size(std::string& err);

    std::string udid_;
    MaaIOSConnectionMethod connection_method_;
    std::string config_;
    MaaIOSScreencapMethod screencap_methods_ = MaaIOSScreencapMethod_Default;
    maa::ios_unit::IOSSession session_;

    int display_width_ = 0;
    int display_height_ = 0;
};

MAA_CTRL_UNIT_NS_END
