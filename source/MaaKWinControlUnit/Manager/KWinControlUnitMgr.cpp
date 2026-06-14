#include "KWinControlUnitMgr.h"

#include <filesystem>
#include <system_error>

#include <opencv2/imgproc.hpp>

#include "Input/UInputController.h"
#include "Screencap/PipeWireScreencap.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

KWinControlUnitMgr::KWinControlUnitMgr(std::filesystem::path device_node, int screen_width, int screen_height)
    : input_(std::make_unique<UInputController>())
    , m_screencap_(std::make_shared<PipeWireScreencap>())
    , device_node_(std::move(device_node))
    , screen_width_(screen_width)
    , screen_height_(screen_height)
{
    LogFunc << VAR(device_node_) << VAR(screen_width_) << VAR(screen_height_);
    m_screencap_->set_screen_size(screen_width_, screen_height_);
}

KWinControlUnitMgr::~KWinControlUnitMgr()
{
    LogFunc;
}

bool KWinControlUnitMgr::connect()
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    LogFunc << VAR(device_node_) << VAR(screen_width_) << VAR(screen_height_);

    // Open uinput input device
    if (!input_->open(device_node_, screen_width_, screen_height_)) {
        LogError << "Failed to open uinput device";
        return false;
    }

    // NOTE: PipeWire screencap initialization is deferred to the first
    // screencap() call (lazy initialization). We do NOT call open() here
    // because:
    //   1. D-Bus handshake (including the KDE auth dialog) should not be
    //      triggered during connect(), only when the user actually needs
    //      a screenshot.
    //   2. PipeWire connection requires a file descriptor from the portal's
    //      OpenPipeWireRemote D-Bus method, which is only available after
    //      the portal Start call completes.
    return true;
}

bool KWinControlUnitMgr::connected() const
{
    if (!input_) {
        return false;
    }
    return input_->connected();
}

bool KWinControlUnitMgr::request_uuid(std::string& uuid)
{
    std::error_code ec;
    std::filesystem::path p(device_node_);
    if (!std::filesystem::exists(p, ec)) {
        return false;
    }

    auto ftime = std::filesystem::last_write_time(p, ec);
    if (ec) {
        return false;
    }

    auto stime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    auto time = std::chrono::system_clock::to_time_t(stime);
    uuid = std::to_string(time);
    return true;
}

MaaControllerFeature KWinControlUnitMgr::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool KWinControlUnitMgr::start_app(const std::string& intent)
{
    // TODO: Implement via D-Bus or system command
    std::ignore = intent;

    return false;
}

bool KWinControlUnitMgr::stop_app(const std::string& intent)
{
    // TODO: Implement via D-Bus or system command
    std::ignore = intent;

    return false;
}

bool KWinControlUnitMgr::screencap(cv::Mat& image)
{
    if (!m_screencap_) {
        LogError << "m_screencap_ is nullptr";
        return false;
    }

    // Delegate directly to PipeWireScreencap.
    // PipeWireScreencap::screencap() will attempt to (re-)initialize if not connected.
    return m_screencap_->screencap(image);
}

bool KWinControlUnitMgr::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool KWinControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool KWinControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool KWinControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool KWinControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_up(contact);
}

bool KWinControlUnitMgr::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool KWinControlUnitMgr::input_text(const std::string& text)
{
    // TODO: Implement keyboard text input via uinput
    std::ignore = text;

    LogError << "input_text not yet implemented for KWin control unit";
    return false;
}

bool KWinControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_down(key);
}

bool KWinControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_up(key);
}

bool KWinControlUnitMgr::relative_move(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->relative_move(dx, dy);
}

bool KWinControlUnitMgr::scroll(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->scroll(dx, dy);
}

bool KWinControlUnitMgr::inactive()
{
    return true;
}

json::object KWinControlUnitMgr::get_info() const
{
    json::object info;
    info["type"] = "kwin";
    info["device_node"] = path_to_utf8_string(device_node_);
    info["screen_width"] = screen_width_;
    info["screen_height"] = screen_height_;
    return info;
}

MAA_CTRL_UNIT_NS_END
