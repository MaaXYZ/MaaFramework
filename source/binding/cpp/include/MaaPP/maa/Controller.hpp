// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <iostream>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/AdbDevice.hpp"
#include "MaaPP/maa/CustomController.hpp"
#include "MaaPP/maa/Exception.hpp"
#include "MaaPP/maa/Image.hpp"
#include "MaaPP/maa/Message.hpp"
#include "MaaPP/maa/Win32Device.hpp"
#include "MaaPP/maa/details/ActionHelper.hpp"
#include "MaaPP/maa/details/Message.hpp"
#include "MaaPP/maa/details/String.hpp"

namespace maa
{

class Controller;

class ControllerAction : public details::ActionBase<ControllerAction, Controller>
{
    friend class Controller;

public:
    using ActionBase::ActionBase;

    MaaStatus status();

    coro::Promise<MaaStatus> wait() { return status_; }

private:
    coro::Promise<MaaStatus> status_;
};

class Controller : public details::ActionHelper<Controller, ControllerAction, MaaControllerHandle>
{
    friend class ControllerAction;
    friend class Instance;

public:
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<Controller>(std::forward<Args>(args)...);
    }

    struct adb_controller_tag
    {
    };

    Controller(
        [[maybe_unused]] adb_controller_tag tag,
        const std::string& adb_path,
        const std::string& address,
        AdbType type,
        const std::string& config,
        const std::string& agent_path,
        std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : ActionHelper(MaaAdbControllerCreateV2(
            adb_path.c_str(),
            address.c_str(),
            type,
            config.c_str(),
            agent_path.c_str(),
            &Controller::_callback,
            this))
        , user_callback_(callback)
    {
    }

    Controller(
        const AdbDevice& device,
        const std::string& agent_path,
        std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : Controller(
            {},
            device.adb_path,
            device.address,
            device.type,
            device.config,
            agent_path,
            callback)
    {
    }

    struct win32_controller_tag
    {
    };

    Controller(
        [[maybe_unused]] win32_controller_tag tag,
        Win32Hwnd hwnd,
        Win32Type type,
        std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : ActionHelper(
            MaaWin32ControllerCreate(hwnd.hwnd_, type.type_, &Controller::_callback, this))
        , user_callback_(callback)
    {
    }

    Controller(
        const Win32Device& device,
        std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : Controller({}, device.hwnd, device.type, callback)
    {
    }

    Controller(
        std::shared_ptr<CustomControllerAPI> api,
        std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : ActionHelper(MaaCustomControllerCreate(
            const_cast<MaaCustomControllerAPI*>(&api_),
            this,
            &Controller::_callback,
            this))
        , user_callback_(callback)
        , custom_ctrl_(api)
    {
    }

    ~Controller()
    {
        MaaControllerDestroy(inst_);
        for (auto action : actions_ | std::views::values) {
            action.lock()->wait().sync_wait();
        }
    }

    std::shared_ptr<Controller> set_long_side(int width)
    {
        if (!MaaControllerSetOption(
                inst_,
                MaaCtrlOption_ScreenshotTargetLongSide,
                &width,
                sizeof(width))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
        return shared_from_this();
    }

    std::shared_ptr<Controller> set_short_side(int width)
    {
        if (!MaaControllerSetOption(
                inst_,
                MaaCtrlOption_ScreenshotTargetShortSide,
                &width,
                sizeof(width))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
        return shared_from_this();
    }

    std::shared_ptr<Controller> set_start_entry(std::string_view entry)
    {
        if (!MaaControllerSetOption(
                inst_,
                MaaCtrlOption_DefaultAppPackageEntry,
                const_cast<char*>(entry.data()),
                entry.size())) {
            throw FunctionFailed("MaaControllerSetOption");
        }
        return shared_from_this();
    }

    std::shared_ptr<Controller> set_stop_entry(std::string_view entry)
    {
        if (!MaaControllerSetOption(
                inst_,
                MaaCtrlOption_DefaultAppPackage,
                const_cast<char*>(entry.data()),
                entry.size())) {
            throw FunctionFailed("MaaControllerSetOption");
        }
        return shared_from_this();
    }

    std::shared_ptr<Controller> set_recording(bool enable)
    {
        if (!MaaControllerSetOption(inst_, MaaCtrlOption_Recording, &enable, sizeof(enable))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
        return shared_from_this();
    }

    std::shared_ptr<ControllerAction> post_connect()
    {
        return put_action(MaaControllerPostConnection(inst_));
    }

    std::shared_ptr<ControllerAction> post_click(int32_t x, int32_t y)
    {
        return put_action(MaaControllerPostClick(inst_, x, y));
    }

    std::shared_ptr<ControllerAction>
        post_swipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration)
    {
        return put_action(MaaControllerPostSwipe(inst_, x1, y1, x2, y2, duration));
    }

    std::shared_ptr<ControllerAction> post_press_key(int32_t key)
    {
        return put_action(MaaControllerPostPressKey(inst_, key));
    }

    std::shared_ptr<ControllerAction> post_input_text(std::string text)
    {
        return put_action(MaaControllerPostInputText(inst_, text.c_str()));
    }

    std::shared_ptr<ControllerAction> post_start_app(std::string intent)
    {
        return put_action(MaaControllerPostStartApp(inst_, intent.c_str()));
    }

    std::shared_ptr<ControllerAction> post_stop_app(std::string intent)
    {
        return put_action(MaaControllerPostStopApp(inst_, intent.c_str()));
    }

    std::shared_ptr<ControllerAction>
        post_touch_down(int32_t contact, int32_t x, int32_t y, int32_t pressure)
    {
        return put_action(MaaControllerPostTouchDown(inst_, contact, x, y, pressure));
    }

    std::shared_ptr<ControllerAction>
        post_touch_move(int32_t contact, int32_t x, int32_t y, int32_t pressure)
    {
        return put_action(MaaControllerPostTouchMove(inst_, contact, x, y, pressure));
    }

    std::shared_ptr<ControllerAction> post_touch_up(int32_t contact)
    {
        return put_action(MaaControllerPostTouchUp(inst_, contact));
    }

    std::shared_ptr<ControllerAction> post_screencap()
    {
        return put_action(MaaControllerPostScreencap(inst_));
    }

    bool connected() { return MaaControllerConnected(inst_); }

    std::shared_ptr<details::Image> image()
    {
        auto img = details::Image::make();
        image(img);
        return img;
    }

    void image(std::shared_ptr<details::Image> img)
    {
        if (!MaaControllerGetImage(inst_, img->handle())) {
            throw FunctionFailed("MaaControllerGetImage");
        }
    }

    std::string uuid()
    {
        details::String buf;
        if (!MaaControllerGetUUID(inst_, buf.handle())) {
            throw FunctionFailed("MaaControllerGetUUID");
        }
        return buf;
    }

private:
    static void _callback(MaaStringView msg, MaaStringView details, MaaTransparentArg arg)
    {
        auto msg_ptr = message::parse(msg, details);

        auto self = reinterpret_cast<Controller*>(arg)->shared_from_this();

        coro::EventLoop::current()->defer([self, msg_ptr]() {
            if (auto msg = msg_ptr->is<message::ControllerActionMessage>()) {
                auto id = msg->id;
                if (!self->actions_.contains(id)) {
                    std::cout << "cannot find id " << id << std::endl;
                    return;
                }
                auto ptr = self->actions_.at(id).lock();
                if (!ptr) {
                    std::cout << "action id " << id << " expired" << std::endl;
                    return;
                }
                if (msg->type == message::ControllerActionMessage::Completed) {
                    ptr->status_.resolve(MaaStatus_Success);
                }
                else if (msg->type == message::ControllerActionMessage::Failed) {
                    ptr->status_.resolve(MaaStatus_Failed);
                }
            }
        });

        if (self->user_callback_) {
            coro::EventLoop::current()->defer([self, msg_ptr]() { self->user_callback_(msg_ptr); });
        }
    }

    static MaaBool _connect(MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->connect().sync_wait();
    }

    static MaaBool _request_uuid(MaaTransparentArg handle_arg, MaaStringBufferHandle buffer)
    {
        details::String buf(buffer);
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        auto res = self->custom_ctrl_->request_uuid().sync_wait();
        if (res.has_value()) {
            buf = res.value();
            return true;
        }
        else {
            return false;
        }
    }

    static MaaBool
        _request_resolution(MaaTransparentArg handle_arg, int32_t* width, int32_t* height)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        auto res = self->custom_ctrl_->request_resolution().sync_wait();
        if (res.has_value()) {
            std::tie(*width, *height) = res.value();
            return true;
        }
        else {
            return false;
        }
    }

    static MaaBool _start_app(MaaStringView intent, MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->start_app(intent).sync_wait();
    }

    static MaaBool _stop_app(MaaStringView intent, MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->stop_app(intent).sync_wait();
    }

    /// Write result to buffer.
    static MaaBool _screencap(MaaTransparentArg handle_arg, /* out */ MaaImageBufferHandle buffer)
    {
        auto buf = details::Image::make(buffer);
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->screencap(buf).sync_wait();
    }

    static MaaBool _click(int32_t x, int32_t y, MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->click(x, y).sync_wait();
    }

    static MaaBool _swipe(
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        int32_t duration,
        MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->swipe(x1, y1, x2, y2, duration).sync_wait();
    }

    static MaaBool _touch_down(
        int32_t contact,
        int32_t x,
        int32_t y,
        int32_t pressure,
        MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->touch_down(contact, x, y, pressure).sync_wait();
    }

    static MaaBool _touch_move(
        int32_t contact,
        int32_t x,
        int32_t y,
        int32_t pressure,
        MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->touch_move(contact, x, y, pressure).sync_wait();
    }

    static MaaBool _touch_up(int32_t contact, MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->touch_up(contact).sync_wait();
    }

    static MaaBool _press_key(int32_t keycode, MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->press_key(keycode).sync_wait();
    }

    static MaaBool _input_text(MaaStringView text, MaaTransparentArg handle_arg)
    {
        auto self = reinterpret_cast<Controller*>(handle_arg)->shared_from_this();
        return self->custom_ctrl_->input_text(text).sync_wait();
    }

    constexpr static MaaCustomControllerAPI api_ = {
        &Controller::_connect,    &Controller::_request_uuid, &Controller::_request_resolution,
        &Controller::_start_app,  &Controller::_stop_app,     &Controller::_screencap,
        &Controller::_click,      &Controller::_swipe,        &Controller::_touch_down,
        &Controller::_touch_move, &Controller::_touch_up,     &Controller::_press_key,
        &Controller::_input_text,
    };

    std::function<void(std::shared_ptr<message::MessageBase>)> user_callback_;
    std::shared_ptr<CustomControllerAPI> custom_ctrl_;
};

inline MaaStatus ControllerAction::status()
{
    return MaaControllerStatus(inst_->inst_, id_);
}

}
