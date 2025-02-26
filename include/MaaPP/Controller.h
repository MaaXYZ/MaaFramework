#pragma once

#include <map>
#include <memory>
#include <vector>

#include <MaaFramework/MaaAPI.h>

#include "./Exception.h"
#include "./Task.h"
#include "./private/Image.h"
#include "./private/String.h"

namespace maapp
{

struct Controller : public std::enable_shared_from_this<Controller>
{
    Controller(MaaController* controller)
    {
        controller_ = controller;
        controller_map_[controller] = weak_from_this();
    }

    Controller(const Controller&) = delete;

    virtual ~Controller()
    {
        controller_map_.erase(controller_);
        MaaControllerDestroy(controller_);
    }

    Controller& operator=(const Controller&) = delete;

    CtrlTask post_connection()
    {
        return {
            controller_,
            MaaControllerPostConnection(controller_),
        };
    }

    bool connected() const { return MaaControllerConnected(controller_); }

    std::vector<uint8_t> cached_image() const
    {
        pri::Image image;
        if (!MaaControllerCachedImage(controller_, image.buffer_)) {
            throw FunctionFailed("MaaControllerCachedImage");
        }
        return image;
    }

    std::string uuid() const
    {
        pri::String uuid;
        if (!MaaControllerGetUuid(controller_, uuid.buffer_)) {
            throw FunctionFailed("MaaControllerGetUuid");
        }
        return uuid;
    }

    void set_screenshot_target_long_side(int size)
    {
        if (!MaaControllerSetOption(controller_, MaaCtrlOption_ScreenshotTargetLongSide, &size, sizeof(size))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
    }

    void set_screenshot_target_short_side(int size)
    {
        if (!MaaControllerSetOption(controller_, MaaCtrlOption_ScreenshotTargetShortSide, &size, sizeof(size))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
    }

    void set_screenshot_use_raw_side(bool flag)
    {
        if (!MaaControllerSetOption(controller_, MaaCtrlOption_ScreenshotUseRawSize, &flag, sizeof(flag))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
    }

    void set_screenshot_recording(bool flag)
    {
        if (!MaaControllerSetOption(controller_, MaaCtrlOption_Recording, &flag, sizeof(flag))) {
            throw FunctionFailed("MaaControllerSetOption");
        }
    }

    MaaController* controller_ {};

    static std::shared_ptr<Controller> find(MaaController* controller)
    {
        auto it = controller_map_.find(controller);
        if (it == controller_map_.end()) {
            return nullptr;
        }
        return it->second.lock();
    }

    static inline std::map<MaaController*, std::weak_ptr<Controller>> controller_map_ {};
};

struct AdbController : public Controller
{
    struct Config
    {
        std::string adb_path;
        std::string address;
        MaaAdbScreencapMethod screencap_methods;
        MaaAdbInputMethod input_methods;
        std::string config;
        std::string agent_path;
    };

    AdbController(const Config& config)
        : Controller(MaaAdbControllerCreate(
              config.adb_path.c_str(),
              config.address.c_str(),
              config.screencap_methods,
              config.input_methods,
              config.config.c_str(),
              config.agent_path.c_str(),
              +[](const char* message, const char* details_json, void* notify_trans_arg) {
                  auto self = static_cast<Controller*>(notify_trans_arg)->shared_from_this();

                  std::ignore = message;
                  std::ignore = details_json;
              },
              this))
    {
    }
};

struct Win32Controller : public Controller
{
    struct Config
    {
        void* hWnd;
        MaaWin32ScreencapMethod screencap_methods;
        MaaWin32InputMethod input_methods;
    };

    Win32Controller(const Config& config)
        : Controller(MaaWin32ControllerCreate(
              config.hWnd,
              config.screencap_methods,
              config.input_methods,
              +[](const char* message, const char* details_json, void* notify_trans_arg) {
                  auto self = static_cast<Controller*>(notify_trans_arg)->shared_from_this();

                  std::ignore = message;
                  std::ignore = details_json;
              },
              this))
    {
    }
};

struct DbgController : public Controller
{
    struct Config
    {
        std::string read_path;
        std::string write_path;
        MaaDbgControllerType type;
        std::string config;
    };

    DbgController(const Config& config)
        : Controller(MaaDbgControllerCreate(
              config.read_path.c_str(),
              config.write_path.c_str(),
              config.type,
              config.config.c_str(),
              +[](const char* message, const char* details_json, void* notify_trans_arg) {
                  auto self = static_cast<Controller*>(notify_trans_arg)->shared_from_this();

                  std::ignore = message;
                  std::ignore = details_json;
              },
              this))
    {
    }
};

}
