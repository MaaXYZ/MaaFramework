#pragma once

#include "ControlUnit/DbgControlUnitAPI.h"

#include <filesystem>

#include <meojson/json.hpp>

#include "Record.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

class ReplayRecording : public ControllerAPI
{
public:
    ReplayRecording(Recording recording) : recording_(std::move(recording)) {}
    virtual ~ReplayRecording() = default;

public: // from ControllerAPI
    virtual std::string uuid() const override { return recording_.device_info.uuid; }
    virtual cv::Size resolution() const override { return recording_.device_info.resolution; }

    virtual bool connect() override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    void sleep(int ms);

private:
    Recording recording_;
    size_t record_index_ = 0;
};

MAA_DBG_CTRL_UNIT_NS_END
