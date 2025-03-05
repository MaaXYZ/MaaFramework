#include "RemoteController.h"

#include "MaaAgent/Message.hpp"
#include "Utils/Codec.h"
#include "Utils/Logger.h"

MAA_AGENT_SERVER_NS_BEGIN

RemoteController::RemoteController(Transceiver& server, const std::string& controller_id)
    : server_(server)
    , controller_id_(controller_id)
{
}

bool RemoteController::set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogError << "Can NOT set option at remote controller" << VAR(key) << VAR_VOIDP(value) << VAR(val_size);
    return false;
}

MaaCtrlId RemoteController::post_connection()
{
    ControllerPostConnectionReverseRequest req {
        .controller_id = controller_id_,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostConnectionReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_click(int x, int y)
{
    ControllerPostClickReverseRequest req {
        .controller_id = controller_id_,
        .x = x,
        .y = y,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostClickReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_swipe(int x1, int y1, int x2, int y2, int duration)
{
    ControllerPostSwipeReverseRequest req {
        .controller_id = controller_id_,
        .x1 = x1,
        .y1 = y1,
        .x2 = x2,
        .y2 = y2,
        .duration = duration,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostSwipeReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_press_key(int keycode)
{
    ControllerPostPressKeyReverseRequest req {
        .controller_id = controller_id_,
        .keycode = keycode,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostPressKeyReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_input_text(const std::string& text)
{
    ControllerPostInputTextReverseRequest req {
        .controller_id = controller_id_,
        .text = text,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostInputTextReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_start_app(const std::string& intent)
{
    ControllerPostStartAppReverseRequest req {
        .controller_id = controller_id_,
        .intent = intent,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostStartAppReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_stop_app(const std::string& intent)
{
    ControllerPostStopAppReverseRequest req {
        .controller_id = controller_id_,
        .intent = intent,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostStopAppReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_screencap()
{
    ControllerPostScreencapReverseRequest req {
        .controller_id = controller_id_,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostScreencapReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_touch_down(int contact, int x, int y, int pressure)
{
    ControllerPostTouchDownReverseRequest req {
        .controller_id = controller_id_,
        .contact = contact,
        .x = x,
        .y = y,
        .pressure = pressure,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostTouchDownReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_touch_move(int contact, int x, int y, int pressure)
{
    ControllerPostTouchMoveReverseRequest req {
        .controller_id = controller_id_,
        .contact = contact,
        .x = x,
        .y = y,
        .pressure = pressure,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostTouchMoveReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaCtrlId RemoteController::post_touch_up(int contact)
{
    ControllerPostTouchUpReverseRequest req {
        .controller_id = controller_id_,
        .contact = contact,
    };
    auto resp_opt = server_.send_and_recv<ControllerPostTouchUpReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->ctrl_id;
}

MaaStatus RemoteController::status(MaaCtrlId ctrl_id) const
{
    ControllerStatusReverseRequest req {
        .controller_id = controller_id_,
        .ctrl_id = ctrl_id,
    };
    auto resp_opt = server_.send_and_recv<ControllerStatusReverseResponse>(req);
    if (!resp_opt) {
        return MaaStatus_Invalid;
    }
    return static_cast<MaaStatus>(resp_opt->status);
}

MaaStatus RemoteController::wait(MaaCtrlId ctrl_id) const
{
    ControllerWaitReverseRequest req {
        .controller_id = controller_id_,
        .ctrl_id = ctrl_id,
    };
    auto resp_opt = server_.send_and_recv<ControllerWaitReverseResponse>(req);
    if (!resp_opt) {
        return MaaStatus_Invalid;
    }
    return static_cast<MaaStatus>(resp_opt->status);
}

bool RemoteController::connected() const
{
    ControllerConnectedReverseRequest req {
        .controller_id = controller_id_,
    };
    auto resp_opt = server_.send_and_recv<ControllerConnectedReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

bool RemoteController::running() const
{
    ControllerRunningReverseRequest req {
        .controller_id = controller_id_,
    };
    auto resp_opt = server_.send_and_recv<ControllerRunningReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

cv::Mat RemoteController::cached_image() const
{
    ControllerCachedImageReverseRequest req {
        .controller_id = controller_id_,
    };
    auto resp_opt = server_.send_and_recv<ControllerCachedImageReverseResponse>(req);
    if (!resp_opt) {
        return {};
    }
    return decode_image(resp_opt->image);
}

std::string RemoteController::get_uuid()
{
    ControllerGetUuidReverseRequest req {
        .controller_id = controller_id_,
    };

    auto resp_opt = server_.send_and_recv<ControllerGetUuidReverseResponse>(req);
    if (!resp_opt) {
        return {};
    }
    return resp_opt->uuid;
}

MAA_AGENT_SERVER_NS_END
