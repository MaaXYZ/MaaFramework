#pragma once

#include <filesystem>

#include <meojson/json.hpp>

#include "ControlUnit/ControlUnitAPI.h"
#include "Record.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ReplayRecording : public ControlUnitAPI
{
public:
    explicit ReplayRecording(Recording recording);

    virtual ~ReplayRecording() override;

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

    virtual bool inactive() override;

private:
    void sleep(int ms);

private:
    Recording recording_;
    size_t record_index_ = 0;
    bool connected_ = false;
};

MAA_CTRL_UNIT_NS_END
