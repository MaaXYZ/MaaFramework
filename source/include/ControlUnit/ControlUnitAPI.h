#pragma once

#include <chrono>
#include <string>
#include <utility>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI() = default;

    virtual bool connect() = 0;

    virtual bool request_uuid(/*out*/ std::string& uuid) = 0;
    virtual MaaControllerFeature get_features() const = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;

    virtual bool screencap(/*out*/ cv::Mat& image) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool scroll(int dx, int dy) = 0;
};

class AdbControlUnitAPI : public ControlUnitAPI
{
public:
    virtual ~AdbControlUnitAPI() = default;

    virtual bool find_device(/*out*/ std::vector<std::string>& devices) = 0;
    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) = 0;
};

class Win32ControlUnitAPI : public ControlUnitAPI
{
public:
    virtual ~Win32ControlUnitAPI() = default;
};

MAA_CTRL_UNIT_NS_END

using MaaControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
using MaaAdbControlUnitHandle = MAA_CTRL_UNIT_NS::AdbControlUnitAPI*;
using MaaWin32ControlUnitHandle = MAA_CTRL_UNIT_NS::Win32ControlUnitAPI*;
using MaaCustomControlUnitHandle = MAA_CTRL_UNIT_NS::ControlUnitAPI*;
