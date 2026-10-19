#include "IOSControlUnitMgr.h"

#include <algorithm>
#include <vector>

#include <MaaUtils/Logger.h>
#include <MaaUtils/NoWarningCV.hpp>

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

constexpr int kDefaultHoldMs = 90;
/// 一次触摸事件之间至少留出的间隔。设备侧靠帧间差值算速度，瞬移式的拖动会被
/// 当成抖动丢掉，所以 duration 再短也要按这个间隔铺开若干步。
constexpr int kMinStepMs = 12;

} // namespace

IOSControlUnitMgr::IOSControlUnitMgr(
    std::string udid,
    std::string wifi_address,
    MaaIOSScreencapMethod screencap_methods,
    MaaIOSConnectionMethod connection_method,
    std::optional<maa::ios::wifi::PairRecord> pairing)
    : udid_(std::move(udid))
    , wifi_address_(std::move(wifi_address))
    , connection_method_(connection_method)
    , pairing_(std::move(pairing))
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
    if (!session_.create(udid_, wifi_address_, screencap_methods_, connection_method_, pairing_, err)) {
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
    // Framework long presses require down/up preferences; these flags do not
    // imply support for additional contacts. Framework pixel scaling stays enabled.
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

bool IOSControlUnitMgr::click(int x, int y)
{
    std::string err;
    double fx = 0, fy = 0;
    if (!normalize(x, y, fx, fy, err)) {
        LogError << "click failed to normalize" << VAR(x) << VAR(y) << VAR(err);
        return false;
    }
    if (!session_.tap(fx, fy, kDefaultHoldMs, err)) {
        LogError << "tap failed" << VAR(fx) << VAR(fy) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    std::string err;
    double fx1 = 0, fy1 = 0, fx2 = 0, fy2 = 0;
    if (!normalize(x1, y1, fx1, fy1, err) || !normalize(x2, y2, fx2, fy2, err)) {
        LogError << "swipe failed to normalize" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(err);
        return false;
    }

    if (duration < 0 || duration > 600000) {
        LogError << "iOS swipe duration must be between 0 and 600000 ms" << VAR(duration);
        return false;
    }
    const int steps = std::max(2, duration / kMinStepMs);
    std::vector<std::pair<double, double>> points;
    points.reserve(static_cast<size_t>(steps) + 1);
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        points.emplace_back(fx1 + (fx2 - fx1) * t, fy1 + (fy2 - fy1) * t);
    }

    const int step_ms = steps > 0 ? std::max(1, duration / steps) : kMinStepMs;
    if (!session_.stroke(points, step_ms, err)) {
        LogError << "stroke failed" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration) << VAR(err);
        return false;
    }
    return true;
}

bool IOSControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    (void)pressure;
    if (contact != 0 || session_.touch_active()) {
        LogError << "iOS controller requires one inactive contact numbered 0" << VAR(contact);
        return false;
    }

    std::string err;
    double fx = 0, fy = 0;
    if (!normalize(x, y, fx, fy, err)) {
        return false;
    }
    return session_.touch(fx, fy, true, err);
}

bool IOSControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    (void)pressure;
    if (contact != 0 || !session_.touch_active()) {
        LogError << "iOS touch_move needs the active contact numbered 0" << VAR(contact);
        return false;
    }

    std::string err;
    double fx = 0, fy = 0;
    if (!normalize(x, y, fx, fy, err)) {
        return false;
    }
    // 每个 CONTACT 报告都是"此刻在此处接触着"，没有 begin/end 操作码，
    // 所以 move 与 down 在线上一模一样。
    return session_.touch(fx, fy, true, err);
}

bool IOSControlUnitMgr::touch_up(int contact)
{
    if (contact != 0) {
        LogError << "iOS controller supports single touch only" << VAR(contact);
        return false;
    }

    std::string err;
    return session_.release_touch(err);
}

bool IOSControlUnitMgr::click_key(int key)
{
    std::string err;
    if (!session_.click_key(key, err)) {
        LogError << "iOS click_key failed" << VAR(key) << VAR(err);
        return false;
    }
    return true;
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
    info["market_name"] = session_.device_property("MarketName");
    info["display_width"] = display_width_;
    info["display_height"] = display_height_;
    return info;
}

MAA_CTRL_UNIT_NS_END
