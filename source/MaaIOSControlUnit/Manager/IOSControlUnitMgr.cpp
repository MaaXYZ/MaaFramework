#include "IOSControlUnitMgr.h"

#include <tuple>

#include <MaaUtils/Logger.h>
#include <MaaUtils/NoWarningCV.hpp>

MAA_CTRL_UNIT_NS_BEGIN

IOSControlUnitMgr::IOSControlUnitMgr(
    std::string udid,
    MaaIOSScreencapMethod screencap_methods,
    MaaIOSConnectionMethod connection_method,
    std::string config)
    : udid_(std::move(udid))
    , connection_method_(connection_method)
    , config_(std::move(config))
    , screencap_methods_(screencap_methods)
{
}

IOSControlUnitMgr::~IOSControlUnitMgr()
{
    session_.close();
}

bool IOSControlUnitMgr::connect()
{
    std::string err;
    if (!session_.create(udid_, screencap_methods_, connection_method_, config_, err)) {
        LogError << "Failed to create iOS session" << VAR(err);
        return false;
    }

    display_width_ = 0;
    display_height_ = 0;

    return true;
}

bool IOSControlUnitMgr::connected() const
{
    return session_.connected();
}

bool IOSControlUnitMgr::request_uuid(std::string& uuid)
{
    uuid = session_.udid();
    return !uuid.empty();
}

MaaControllerFeature IOSControlUnitMgr::get_features() const
{
    // Framework long presses use down/up; pixel scaling stays enabled.
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool IOSControlUnitMgr::start_app(const std::string& intent)
{
    // intent 在这里就是 bundle id（com.apple.mobilesafari 这种），和 Android 那边
    // 传包名是同一个位置。语义也对齐：Android 用 `monkey -p <pkg> 1` 唤起，不动
    // 已经在跑的实例，所以这边 terminateExisting 传 false。
    std::string err;
    if (!session_.launch_app(intent, err)) {
        LogError << "start_app failed" << VAR(intent) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::stop_app(const std::string& intent)
{
    std::string err;
    if (!session_.stop_app(intent, err)) {
        LogError << "stop_app failed" << VAR(intent) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::refresh_display_size(std::string& err)
{
    cv::Mat image;
    if (!session_.screencap(image, err)) {
        return false;
    }
    display_width_ = image.cols;
    display_height_ = image.rows;
    LogInfo << "iOS display size" << VAR(display_width_) << VAR(display_height_);
    return true;
}

bool IOSControlUnitMgr::normalize(int x, int y, double& fx, double& fy, std::string& err)
{
    if (display_width_ <= 0 || display_height_ <= 0) {
        if (!refresh_display_size(err)) {
            LogError << "cannot learn display size" << VAR(err);
            return false;
        }
    }
    // 出界只警告不报错：坐标来自框架，正常路径不会越界，真越界了也照样夹进
    // 触摸面（设备侧本来就会夹），但要把尺寸对不上的事实留在日志里。
    if (x < 0 || y < 0 || x >= display_width_ || y >= display_height_) {
        LogWarn << "touch point outside the screenshot" << VAR(x) << VAR(y) << VAR(display_width_) << VAR(display_height_);
    }
    // 换向交给会话：它才知道设备报的界面朝向，而触摸面的 0..1 是面板轴的。
    return session_.logical_to_panel(x, y, display_width_, display_height_, fx, fy, err);
}

bool IOSControlUnitMgr::screencap(cv::Mat& image)
{
    std::string err;
    if (!session_.screencap(image, err)) {
        LogError << "screencap failed" << VAR(err);
        return false;
    }

    display_width_ = image.cols;
    display_height_ = image.rows;
    return true;
}

// ControllerAgent uses touch_down/touch_move/touch_up when the feature flag is set.
bool IOSControlUnitMgr::click(int x, int y)
{
    LogError << "deprecated: use touch_down/touch_up instead" << VAR(x) << VAR(y);
    return false;
}

bool IOSControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: use touch_down/touch_move/touch_up instead" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool IOSControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;
    if (contact < 0 || contact >= maa::ios_unit::kMaxTouchContacts || session_.touch_active(contact)) {
        LogError << "iOS touch_down requires an inactive contact in [0,4]" << VAR(contact);
        return false;
    }

    std::string err;
    double fx = 0, fy = 0;
    if (!normalize(x, y, fx, fy, err)) {
        LogError << "iOS touch_down failed to normalize" << VAR(contact) << VAR(x) << VAR(y) << VAR(err);
        return false;
    }
    if (!session_.touch(contact, fx, fy, true, err)) {
        LogError << "iOS touch_down failed" << VAR(contact) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;
    if (contact < 0 || contact >= maa::ios_unit::kMaxTouchContacts || !session_.touch_active(contact)) {
        LogError << "iOS touch_move requires an active contact in [0,4]" << VAR(contact);
        return false;
    }

    std::string err;
    double fx = 0, fy = 0;
    if (!normalize(x, y, fx, fy, err)) {
        LogError << "iOS touch_move failed to normalize" << VAR(contact) << VAR(x) << VAR(y) << VAR(err);
        return false;
    }
    // 每个 CONTACT 报告都是"此刻在此处接触着"，没有 begin/end 操作码，
    // 所以 move 与 down 在线上一模一样。
    if (!session_.touch(contact, fx, fy, true, err)) {
        LogError << "iOS touch_move failed" << VAR(contact) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::touch_up(int contact)
{
    if (contact < 0 || contact >= maa::ios_unit::kMaxTouchContacts) {
        LogError << "iOS touch_up requires a contact in [0,4]" << VAR(contact);
        return false;
    }

    std::string err;
    if (!session_.release_touch(contact, err)) {
        LogError << "iOS touch_up failed" << VAR(contact) << VAR(err);
        return false;
    }
    return true;
}

// ControllerAgent uses key_down/key_up when the feature flag is set.
bool IOSControlUnitMgr::click_key(int key)
{
    LogError << "deprecated: use key_down/key_up instead" << VAR(key);
    return false;
}

bool IOSControlUnitMgr::input_text(const std::string& text)
{
    std::string err;
    if (!session_.type_text(text, err)) {
        LogError << "input_text failed" << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::key_down(int key)
{
    std::string err;
    if (!session_.key_down(key, err)) {
        LogError << "iOS key_down failed" << VAR(key) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::key_up(int key)
{
    std::string err;
    if (!session_.key_up(key, err)) {
        LogError << "iOS key_up failed" << VAR(key) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::inactive()
{
    std::string err;
    if (!session_.release_inputs(err)) {
        LogError << "iOS input cleanup failed" << VAR(err);
        return false;
    }
    return true;
}

json::object IOSControlUnitMgr::get_info() const
{
    json::object info;
    // type 不是装饰：MaaControllerGetInfo 会校验它，缺了就整条 info 报
    // "missing or invalid type field" 并返回失败（各控制单元都靠这个字段自报家门）。
    info["type"] = "ios";
    info["udid"] = session_.udid();
    info["product_type"] = session_.device_property("ProductType");
    info["os_version"] = session_.device_property("OSVersion");
    info["display_width"] = display_width_;
    info["display_height"] = display_height_;
    return info;
}

MAA_CTRL_UNIT_NS_END
