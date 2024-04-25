#pragma once

#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/AdbDevice.hpp"
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

    ~Controller() { MaaControllerDestroy(inst_); }

    bool set_long_side(int width)
    {
        return MaaControllerSetOption(
            inst_,
            MaaCtrlOption_ScreenshotTargetLongSide,
            &width,
            sizeof(width));
    }

    bool set_short_side(int width)
    {
        return MaaControllerSetOption(
            inst_,
            MaaCtrlOption_ScreenshotTargetShortSide,
            &width,
            sizeof(width));
    }

    bool set_start_entry(std::string_view entry)
    {
        return MaaControllerSetOption(
            inst_,
            MaaCtrlOption_DefaultAppPackageEntry,
            const_cast<char*>(entry.data()),
            entry.size());
    }

    bool set_stop_entry(std::string_view entry)
    {
        return MaaControllerSetOption(
            inst_,
            MaaCtrlOption_DefaultAppPackage,
            const_cast<char*>(entry.data()),
            entry.size());
    }

    bool set_recording(bool enable)
    {
        return MaaControllerSetOption(inst_, MaaCtrlOption_Recording, &enable, sizeof(enable));
    }

    std::shared_ptr<ControllerAction> post_connect()
    {
        return put_action(MaaControllerPostConnection(inst_));
    }

    bool connected() { return MaaControllerConnected(inst_); }

    std::optional<std::string> uuid()
    {
        details::String buf;
        if (MaaControllerGetUUID(inst_, buf.handle())) {
            return buf;
        }
        else {
            return std::nullopt;
        }
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

    std::function<void(std::shared_ptr<message::MessageBase>)> user_callback_;
};

inline MaaStatus ControllerAction::status()
{
    return MaaControllerStatus(inst_->inst_, id_);
}

}
