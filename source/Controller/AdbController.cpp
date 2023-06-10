#include "AdbController.h"

#include "Platform/PlatformFactory.h"
#include "Resource/AdbConfig.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

MAA_CTRL_NS_BEGIN

AdbController::AdbController(const std::string& adb_path, const std::string& address, MaaControllerCallback callback,
                             void* callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(adb_path), address_(address)
{
    platform_io_ = PlatformFactory::create();
    support_socket_ = platform_io_->support_socket_;

    if (!support_socket_) {
        LogError << "AdbController not support socket";
    }
}

AdbController::~AdbController() {}

bool AdbController::_connect()
{
    auto cfg = adb_cfg();
    if (!cfg) {
        LogError << "adb_cfg is nullptr";
        return false;
    }

    const auto& raw_cmd = cfg->data().connect;
    std::string cmd = string_replace_all(raw_cmd, {
                                                      { "{ADB}", adb_path_ },
                                                      { "{ADB_SERIAL}", address_ },
                                                  });

    std::string pipe_data;
    std::string sock_data;
    auto cmd_ret = command(cmd, false, 60LL * 1000);

    bool success = cmd_ret ? cmd_ret->find("error") == std::string::npos : false;

    notifier.notify(success ? MaaMsg::Connected : MaaMsg::ConnectFailed);

    return success;
}

void AdbController::_click(ClickParams param) {}

void AdbController::_swipe(SwipeParams param) {}

cv::Mat AdbController::_screencap()
{
    return cv::Mat();
}

std::shared_ptr<MAA_RES_NS::AdbConfig> AdbController::adb_cfg() const
{
    auto* res = resource();
    if (!res) {
        LogError << "resource is nullptr";
        return nullptr;
    }
    return res->adb_cfg();
}

std::optional<std::string> AdbController::command(const std::string& cmd, bool recv_by_socket, int64_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();

    std::string pipe_data;
    std::string sock_data;
    int ret = platform_io_->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout);

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    LogInfo << VAR(cmd) << VAR(ret) << VAR(pipe_data.size()) << VAR(sock_data.size()) << VAR(duration);
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        LogTrace << "stdout output:" << Logger::separator::newline << pipe_data;
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        LogTrace << "socket output:" << Logger::separator::newline << sock_data;
    }

    if (ret != 0) {
        LogError << "call_command failed" << VAR(cmd) << VAR(ret);
        return std::nullopt;
    }

    return recv_by_socket ? sock_data : pipe_data;
}

MAA_CTRL_NS_END
