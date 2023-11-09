#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include <filesystem>

#include <meojson/json.hpp>

MAA_CTRL_UNIT_NS_BEGIN

class CarouselImage : public ControlUnitAPI
{
public:
    CarouselImage(std::filesystem::path path) : path_(std::move(path)) {}
    virtual ~CarouselImage() = default;

public: // from ControlUnitAPI
    virtual bool find_device(/*out*/ std::vector<std::string>& devices) override;

    virtual bool connect() override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;
    virtual bool request_resolution(/*out*/ int& width, /*out*/ int& height) override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;

private:
    std::filesystem::path path_;
    std::vector<cv::Mat> images_;
    size_t image_index_ = 0;
    cv::Size resolution_ {};
};

MAA_CTRL_UNIT_NS_END
