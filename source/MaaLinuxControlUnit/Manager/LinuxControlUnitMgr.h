#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#include "Common/Conf.h"

#include "Base/Config.h"
#include "Wayland/WaylandClient.h"

MAA_CTRL_UNIT_NS_BEGIN

class LinuxControlUnitMgr : public LinuxControlUnitAPI
{
public:
    LinuxControlUnitMgr(const LinuxControlUnitConfig& config);
    virtual ~LinuxControlUnitMgr() override;

public:
    virtual bool connect() override;
    virtual bool connected() const override;

    virtual bool request_uuid(std::string& uuid) override;
    virtual MaaControllerFeature get_features() const override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(cv::Mat& image) override;

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
    virtual bool relative_move(int dx, int dy) override;

    virtual bool inactive() override;

    virtual json::object get_info() const override;

private:
    bool init_screencap();
    bool init_input();
    bool create_wl_client();

private:
    const LinuxControlUnitConfig config_;
    bool connected_ = false;
    std::shared_ptr<InputBase> input_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;

    std::shared_ptr<WaylandClient> wl_client_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
