#include "WlRootsControlUnitMgr.h"

#include <filesystem>
#include <system_error>

#include <opencv2/imgproc.hpp>

#include "Client/VkToEvdev.h"
#include "Client/WaylandClient.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

WlRootsControlUnitMgr::WlRootsControlUnitMgr(std::filesystem::path wlr_socket_path, bool use_win32_vk_code)
    : client_(std::make_unique<WaylandClient>())
    , wlr_socket_path_(std::move(wlr_socket_path))
    , use_win32_vk_code_(use_win32_vk_code)
{
    LogFunc << VAR(wlr_socket_path_) << VAR(use_win32_vk_code_);
}

WlRootsControlUnitMgr::~WlRootsControlUnitMgr()
{
    LogFunc;
}

bool WlRootsControlUnitMgr::connect()
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    LogFunc << VAR(wlr_socket_path_);
    return client_->connect(wlr_socket_path_);
}

bool WlRootsControlUnitMgr::connected() const
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }
    return client_->connected();
}

bool WlRootsControlUnitMgr::request_uuid(std::string& uuid)
{
    std::error_code ec;
    std::filesystem::path p(wlr_socket_path_);
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

MaaControllerFeature WlRootsControlUnitMgr::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool WlRootsControlUnitMgr::start_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool WlRootsControlUnitMgr::screencap(cv::Mat& image)
{
    uint32_t width, height = 0;
    uint32_t format = 0; // format 0=argb8888 1=xrgb8888
    void* buffer;

    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    if (!client_->screencap(&buffer, width, height, format)) {
        LogError << "Failed to screencap";
        return false;
    }
    int cvt_mode = cv::COLOR_RGBA2BGR;
    switch (format) { // TODO: Other possible format?
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
        cvt_mode = cv::COLOR_RGBA2BGR;
        break;
    case WL_SHM_FORMAT_ARGB8888:
    case WL_SHM_FORMAT_XRGB8888:
        cvt_mode = cv::COLOR_BGRA2BGR;
        break;
    default:
        LogWarn << "Unsupported color format" << VAR(format) << "Using RGBA2BGR";
    }
    LogDebug << "Converting buffer" << VAR(format) << VAR(cvt_mode);

    const cv::Mat raw(height, width, CV_8UC4, buffer);

    cv::cvtColor(raw, image, cvt_mode);
    return true;
}

bool WlRootsControlUnitMgr::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool WlRootsControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool WlRootsControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    return client_->pointer(WaylandClient::EventPhase::Began, x, y, contact);
}

bool WlRootsControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    return client_->pointer(WaylandClient::EventPhase::Moved, x, y, contact);
}

bool WlRootsControlUnitMgr::touch_up(int contact)
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    // Ended phase only sends button release; WaylandClient ignores x/y. (0,0) is a placeholder.
    return client_->pointer(WaylandClient::EventPhase::Ended, 0, 0, contact);
}

bool WlRootsControlUnitMgr::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool WlRootsControlUnitMgr::input_text(const std::string& text)
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    return client_->input_str(text);
}

bool WlRootsControlUnitMgr::key_down(int key)
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    const int evdev_key = translate_key(key);
    if (evdev_key == 0) {
        LogError << "Failed to translate key" << VAR(key) << VAR(use_win32_vk_code_);
        return false;
    }

    return client_->input_key(WaylandClient::EventPhase::Began, evdev_key);
}

bool WlRootsControlUnitMgr::key_up(int key)
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    const int evdev_key = translate_key(key);
    if (evdev_key == 0) {
        LogError << "Failed to translate key" << VAR(key) << VAR(use_win32_vk_code_);
        return false;
    }

    return client_->input_key(WaylandClient::EventPhase::Ended, evdev_key);
}

int WlRootsControlUnitMgr::translate_key(int key) const
{
    if (!use_win32_vk_code_) {
        return key;
    }

    const int evdev_key = vk_to_evdev(key);
    LogDebug << "Translating VK to evdev" << VAR(key) << VAR(evdev_key);
    return evdev_key;
}

bool WlRootsControlUnitMgr::relative_move(int dx, int dy)
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    return client_->relative_move(dx, dy);
}

bool WlRootsControlUnitMgr::scroll(int dx, int dy)
{
    if (!client_) {
        LogError << "client_ is nullptr";
        return false;
    }

    return client_->scroll(dx, dy);
}

bool WlRootsControlUnitMgr::stop_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool WlRootsControlUnitMgr::inactive()
{
    return true;
}

json::object WlRootsControlUnitMgr::get_info() const
{
    json::object info;
    info["type"] = "wlroots";
    info["wlr_socket_path"] = path_to_utf8_string(wlr_socket_path_);
    info["use_win32_vk_code"] = use_win32_vk_code_;
    return info;
}

MAA_CTRL_UNIT_NS_END
