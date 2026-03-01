#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class WaylandClient;

class WlRootsControlUnitMgr : public ControlUnitAPI
{
public:
    WlRootsControlUnitMgr(std::filesystem::path wlr_socket_path);
    virtual ~WlRootsControlUnitMgr() override;

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

    virtual bool inactive() override;

private:
    std::unique_ptr<WaylandClient> client_;
    std::filesystem::path wlr_socket_path_;
    std::pair<int, int> last_pos_ = { 0, 0 };
    bool has_clicked_ = false;
};

MAA_CTRL_UNIT_NS_END
