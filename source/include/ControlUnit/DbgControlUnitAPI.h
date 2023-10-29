#pragma once

#include <memory>
#include <optional>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_DBG_CTRL_UNIT_NS_BEGIN

class ControllerAPI
{
public:
    virtual ~ControllerAPI() = default;

    virtual bool connect() = 0;

    virtual std::string uuid() const = 0;
    virtual cv::Size resolution() const = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool press_key(int key) = 0;

    virtual std::optional<cv::Mat> screencap() = 0;
};

std::shared_ptr<ControllerAPI> MAA_CONTROL_UNIT_API create_controller(MaaDbgControllerType type,
                                                                      MaaStringView read_path, MaaStringView write_path,
                                                                      MaaStringView config);

MAA_DBG_CTRL_UNIT_NS_END
