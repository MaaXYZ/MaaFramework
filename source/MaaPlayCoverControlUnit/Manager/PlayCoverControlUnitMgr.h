#pragma once

#include <memory>
#include <string>
#include <utility>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class PlayToolsClient;

class PlayCoverControlUnitMgr : public ControlUnitAPI
{
public:
    PlayCoverControlUnitMgr(std::string address, std::string uuid);
    virtual ~PlayCoverControlUnitMgr() override;

public:
    virtual bool connect() override;

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

private:
    std::unique_ptr<PlayToolsClient> client_;
    std::string address_;
    std::string uuid_;
    int last_touch_x_ = 0;
    int last_touch_y_ = 0;
    bool has_last_touch_point_ = false;
};

MAA_CTRL_UNIT_NS_END
