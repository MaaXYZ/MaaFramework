#include "LinuxControlUnitMgr.h"

#include "Input/UInput.h"
#include "Input/WlrInput.h"
#include "Screencap/PipeWireScreencap.h"
#include "Screencap/WlrScreencap.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

LinuxControlUnitMgr::LinuxControlUnitMgr(const LinuxControlUnitConfig& config)
    : config_(config)
{
}

LinuxControlUnitMgr::~LinuxControlUnitMgr()
{
}

bool LinuxControlUnitMgr::connect()
{
    connected_ = false;
    screencap_.reset();
    input_.reset();

    if (config_.screencap_method == MaaLinuxScreencapMethod_Wlr || config_.input_method == MaaLinuxInputMethod_Wlr) {
        if (!create_wl_client()) {
            LogError << "Failed to create WaylandClient";
            return false;
        }
    }

    if (!init_screencap()) {
        return false;
    }
    if (!init_input()) {
        return false;
    }

    connected_ = true;
    return true;
}

bool LinuxControlUnitMgr::connected() const
{
    return connected_;
}

bool LinuxControlUnitMgr::request_uuid(std::string& uuid)
{
    std::stringstream ss;
    switch (config_.screencap_method) {
    case MaaLinuxScreencapMethod_Wlr:
    case MaaLinuxScreencapMethod_ExtImage:
        ss << config_.wlr_socket_path;
        break;
    case MaaLinuxScreencapMethod_PipeWire:
        ss << config_.pw_socket_fd;
        break;
    default:
        ss << "-1";
        break;
    }
    ss << '_';
    switch (config_.input_method) {
    case MaaLinuxInputMethod_Wlr:
        ss << config_.wlr_socket_path;
        break;
    case MaaLinuxInputMethod_UInput:
        ss << config_.uinput_path;
        break;
    default:
        ss << "-1";
        break;
    }

    uuid = std::move(ss).str();

    return true;
}

MaaControllerFeature LinuxControlUnitMgr::get_features() const
{
    MaaControllerFeature feat = MaaControllerFeature_None;
    if (input_) {
        feat |= input_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    }
    return feat;
}

bool LinuxControlUnitMgr::start_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool LinuxControlUnitMgr::stop_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool LinuxControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is null";
        return false;
    }

    auto opt = screencap_->screencap();
    if (!opt) {
        LogError << "failed to screencap";
        return false;
    }

    image = std::move(opt).value();

    return true;
}

bool LinuxControlUnitMgr::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click(x, y);
}

bool LinuxControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool LinuxControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool LinuxControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool LinuxControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_up(contact);
}

bool LinuxControlUnitMgr::click_key(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click_key(key);
}

bool LinuxControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->input_text(text);
}

bool LinuxControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->key_down(key);
}

bool LinuxControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->key_up(key);
}

bool LinuxControlUnitMgr::scroll(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->scroll(dx, dy);
}

bool LinuxControlUnitMgr::relative_move(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    auto relative_move_input = std::dynamic_pointer_cast<RelativeMoveInput>(input_);
    if (!relative_move_input) {
        LogError << "relative_move is not supported by the current input method";
        return false;
    }

    return relative_move_input->relative_move(dx, dy);
}

bool LinuxControlUnitMgr::inactive()
{
    LogFunc;

    if (screencap_) {
        screencap_->inactive();
    }
    if (input_) {
        input_->inactive();
    }

    return true;
}

json::object LinuxControlUnitMgr::get_info() const
{
    return config_.to_json().as_object();
}

bool LinuxControlUnitMgr::init_screencap()
{
    switch (config_.screencap_method) {
    case MaaLinuxScreencapMethod_Wlr:
        screencap_ = std::make_shared<WlrScreencap>(wl_client_);
        return true;
    case MaaLinuxScreencapMethod_PipeWire:
        screencap_ = std::make_shared<PipeWireScreencap>(
            config_.pw_socket_fd,
            config_.pw_node_id,
            config_.pw_screen_width,
            config_.pw_screen_height);
        return true;
    default:
        LogError << "Unknown Screencap Method" << VAR(config_.screencap_method);
        return false;
    }
}

bool LinuxControlUnitMgr::init_input()
{
    switch (config_.input_method) {
    case MaaLinuxInputMethod_Wlr:
        input_ = std::make_shared<WlrInput>(wl_client_);
        return true;
    case MaaLinuxInputMethod_UInput:
        input_ = std::make_shared<UInput>();
        return true;
    default:
        LogError << "Unknown Input Method" << VAR(config_.input_method);
        return false;
    }
}

bool LinuxControlUnitMgr::create_wl_client()
{
    if (wl_client_ != nullptr) {
        return true;
    }
    wl_client_ = std::make_shared<WaylandClient>();
    return wl_client_->connect(config_.wlr_socket_path);
}

MAA_CTRL_UNIT_NS_END
