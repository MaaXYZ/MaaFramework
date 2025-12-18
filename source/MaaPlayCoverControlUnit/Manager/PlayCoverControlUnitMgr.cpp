#include "PlayCoverControlUnitMgr.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"

#include "Client/PlayToolsClient.h"

MAA_CTRL_UNIT_NS_BEGIN

PlayCoverControlUnitMgr::PlayCoverControlUnitMgr(std::string address, std::string uuid)
    : client_(std::make_unique<PlayToolsClient>())
    , address_(std::move(address))
    , uuid_(std::move(uuid))
{
    LogFunc << VAR(address_) << VAR(uuid_);
}

PlayCoverControlUnitMgr::~PlayCoverControlUnitMgr()
{
    LogFunc;
}

bool PlayCoverControlUnitMgr::connect()
{
    LogFunc << VAR(address_);
    return client_->connect(address_);
}

bool PlayCoverControlUnitMgr::request_uuid(std::string& uuid)
{
    uuid = uuid_;
    return true;
}

MaaControllerFeature PlayCoverControlUnitMgr::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
}

bool PlayCoverControlUnitMgr::start_app(const std::string& intent)
{
    LogWarn << "start_app is not supported on PlayCover" << VAR(intent);
    std::ignore = intent;
    return false;
}

bool PlayCoverControlUnitMgr::stop_app(const std::string& intent)
{
    LogInfo << "stop_app" << VAR(intent);
    std::ignore = intent;
    return client_->terminate();
}

bool PlayCoverControlUnitMgr::screencap(cv::Mat& image)
{
    LogTrace;

    std::vector<uint8_t> buffer;
    int width = 0, height = 0;

    if (!client_->screencap(buffer, width, height)) {
        LogError << "Failed to screencap";
        return false;
    }

    if (buffer.empty() || width <= 0 || height <= 0) {
        LogError << "Invalid screencap data";
        return false;
    }

    cv::Mat rgba(height, width, CV_8UC4, buffer.data());
    cv::cvtColor(rgba, image, cv::COLOR_RGBA2BGR);

    return true;
}

bool PlayCoverControlUnitMgr::click(int x, int y)
{
    LogTrace << VAR(x) << VAR(y);

    if (!touch_down(0, x, y, 0)) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DefaultClickDelay));

    return touch_up(0);
}

bool PlayCoverControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogTrace << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    auto [width, height] = client_->screen_size();
    if (width <= 0 || height <= 0) {
        LogError << "Invalid screen size";
        return false;
    }

    x1 = std::clamp(x1, 0, width - 1);
    y1 = std::clamp(y1, 0, height - 1);

    if (!touch_down(0, x1, y1, 0)) {
        return false;
    }

    int actual_duration = duration > 0 ? duration : DefaultSwipeDuration;

    auto cubic_spline = [](double slope_0, double slope_1, double t) {
        const double a = slope_0;
        const double b = -(2 * slope_0 + slope_1 - 3);
        const double c = -(-slope_0 - slope_1 + 2);
        return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
    };

    constexpr double slope_in = 1.0;
    constexpr double slope_out = 1.0;

    for (int cur_time = DefaultSwipeDelay; cur_time < actual_duration; cur_time += DefaultSwipeDelay) {
        double progress = cubic_spline(slope_in, slope_out, static_cast<double>(cur_time) / actual_duration);
        int cur_x = static_cast<int>(std::lerp(static_cast<double>(x1), static_cast<double>(x2), progress));
        int cur_y = static_cast<int>(std::lerp(static_cast<double>(y1), static_cast<double>(y2), progress));

        if (cur_x < 0 || cur_x >= width || cur_y < 0 || cur_y >= height) {
            continue;
        }

        if (!touch_move(0, cur_x, cur_y, 0)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(DefaultSwipeDelay));
    }

    if (x2 >= 0 && x2 < width && y2 >= 0 && y2 < height) {
        if (!touch_move(0, x2, y2, 0)) {
            return false;
        }
    }

    return touch_up(0);
}

bool PlayCoverControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    if (contact != 0) {
        LogWarn << "PlayCover only supports single touch, contact:" << contact;
        return false;
    }

    if (!client_->touch(PlayToolsClient::TouchPhase::Began, x, y)) {
        return false;
    }

    last_touch_x_ = x;
    last_touch_y_ = y;
    has_last_touch_point_ = true;
    return true;
}

bool PlayCoverControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    if (contact != 0) {
        LogWarn << "PlayCover only supports single touch, contact:" << contact;
        return false;
    }

    if (!client_->touch(PlayToolsClient::TouchPhase::Moved, x, y)) {
        return false;
    }

    last_touch_x_ = x;
    last_touch_y_ = y;
    has_last_touch_point_ = true;
    return true;
}

bool PlayCoverControlUnitMgr::touch_up(int contact)
{
    if (contact != 0) {
        LogWarn << "PlayCover only supports single touch, contact:" << contact;
        return false;
    }

    int up_x = 0;
    int up_y = 0;
    if (has_last_touch_point_) {
        up_x = last_touch_x_;
        up_y = last_touch_y_;
    }
    else {
        auto [width, height] = client_->screen_size();
        up_x = width / 2;
        up_y = height / 2;
    }

    if (!client_->touch(PlayToolsClient::TouchPhase::Ended, up_x, up_y)) {
        return false;
    }

    has_last_touch_point_ = false;
    return true;
}

bool PlayCoverControlUnitMgr::click_key(int key)
{
    LogWarn << "click_key is not supported on PlayCover" << VAR(key);
    std::ignore = key;
    return false;
}

bool PlayCoverControlUnitMgr::input_text(const std::string& text)
{
    LogWarn << "input_text is not supported on PlayCover" << VAR(text);
    std::ignore = text;
    return false;
}

bool PlayCoverControlUnitMgr::key_down(int key)
{
    LogWarn << "key_down is not supported on PlayCover" << VAR(key);
    std::ignore = key;
    return false;
}

bool PlayCoverControlUnitMgr::key_up(int key)
{
    LogWarn << "key_up is not supported on PlayCover" << VAR(key);
    std::ignore = key;
    return false;
}

bool PlayCoverControlUnitMgr::scroll(int dx, int dy)
{
    LogWarn << "scroll is not supported on PlayCover" << VAR(dx) << VAR(dy);
    std::ignore = dx;
    std::ignore = dy;
    return false;
}

MAA_CTRL_UNIT_NS_END
