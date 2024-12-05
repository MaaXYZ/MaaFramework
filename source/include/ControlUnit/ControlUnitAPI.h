#pragma once

#include <string>
#include <utility>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI() = default;

    virtual bool find_device(/*out*/ std::vector<std::string>& devices) = 0;

    virtual bool connect() = 0;

    virtual bool request_uuid(/*out*/ std::string& uuid) = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;

    virtual bool screencap(/*out*/ cv::Mat& image) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    struct SwipeParam
    {
        int x1 = 0;
        int y1 = 0;
        int x2 = 0;
        int y2 = 0;
        int duration = 0;
        int starting = 0;
    };

    virtual bool multi_swipe(const std::vector<SwipeParam>& swipes) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool press_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;
};

MAA_CTRL_UNIT_NS_END

using MaaControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
