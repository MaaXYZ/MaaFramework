#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class UInputController;
class PipeWireScreencap;

class KWinControlUnitMgr : public KWinControlUnitAPI
{
public:
    KWinControlUnitMgr(std::filesystem::path device_node, int screen_width, int screen_height, bool use_win32_vk_code);
    virtual ~KWinControlUnitMgr() override;

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

    virtual bool relative_move(int dx, int dy) override;
    virtual bool scroll(int dx, int dy) override;

    virtual bool inactive() override;

    virtual json::object get_info() const override;

private:
    int translate_key(int key) const;

    std::unique_ptr<UInputController> input_;
    std::shared_ptr<PipeWireScreencap> m_screencap_;
    std::filesystem::path device_node_;
    int screen_width_ = 0;
    int screen_height_ = 0;
    const bool use_win32_vk_code_ = false;
};

MAA_CTRL_UNIT_NS_END
