#pragma once

#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <variant>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "Utils/MessageNotifier.hpp"
#include "Utils/NoWarningCVMat.hpp"

MAA_RES_NS_BEGIN
class ResourceMgr;
MAA_RES_NS_END

MAA_CTRL_NS_BEGIN

struct ClickParam
{
    int x = 0;
    int y = 0;

    MEO_JSONIZATION(x, y);
};

struct SwipeParam
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int duration = 0;
    int starting = 0;

    MEO_JSONIZATION(x1, y1, x2, y2, duration, starting);
};

struct TouchParam
{
    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;

    MEO_JSONIZATION(contact, x, y, pressure);
};

struct PressKeyParam
{
    int keycode = 0;

    MEO_JSONIZATION(keycode);
};

struct InputTextParam
{
    std::string text;

    MEO_JSONIZATION(text);
};

struct AppParam
{
    std::string package;

    MEO_JSONIZATION(package);
};

using Param =
    std::variant<std::monostate, ClickParam, SwipeParam, std::vector<SwipeParam>, TouchParam, PressKeyParam, InputTextParam, AppParam>;

struct Action
{
    enum class Type
    {
        invalid,
        connect,
        click,
        swipe,
        multi_swipe,
        touch_down,
        touch_move,
        touch_up,
        press_key,
        input_text,
        screencap,
        start_app,
        stop_app,
    } type = Type::invalid;

    Param param;
};

std::ostream& operator<<(std::ostream& os, const Action& action);

class ControllerAgent : public MaaController
{
public:
    ControllerAgent(MaaNotificationCallback notify, void* notify_trans_arg);
    virtual ~ControllerAgent() override;

public: // MaaController
    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaCtrlId post_connection() override;
    virtual MaaCtrlId post_click(int x, int y) override;
    virtual MaaCtrlId post_swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual MaaCtrlId post_press_key(int keycode) override;
    virtual MaaCtrlId post_input_text(const std::string& text) override;
    virtual MaaCtrlId post_start_app(const std::string& intent) override;
    virtual MaaCtrlId post_stop_app(const std::string& intent) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_up(int contact) override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const override;
    virtual bool connected() const override;
    virtual bool running() const override;

    virtual cv::Mat cached_image() const override;
    virtual std::string get_uuid() override;

public:
    void post_stop();

    bool click(const cv::Rect& r);
    bool click(const cv::Point& p);
    bool swipe(const cv::Rect& r1, const cv::Rect& r2, int duration);
    bool swipe(const cv::Point& p1, const cv::Point& p2, int duration);

    struct SwipeParamWithRect
    {
        cv::Rect r1 {};
        cv::Rect r2 {};
        uint duration = 0;
        uint starting = 0;
    };

    bool multi_swipe(const std::vector<SwipeParamWithRect>& swipes);

    bool press_key(int keycode);
    bool input_text(const std::string& text);
    cv::Mat screencap();

    bool start_app(const std::string& package);
    bool stop_app(const std::string& package);

protected:
    virtual bool _connect() = 0;
    virtual std::optional<std::string> _request_uuid() = 0;
    virtual bool _start_app(AppParam param) = 0;
    virtual bool _stop_app(AppParam param) = 0;
    virtual std::optional<cv::Mat> _screencap() = 0;
    virtual bool _click(ClickParam param) = 0;
    virtual bool _swipe(SwipeParam param) = 0;
    virtual bool _multi_swipe(std::vector<SwipeParam> param) = 0;
    virtual bool _touch_down(TouchParam param) = 0;
    virtual bool _touch_move(TouchParam param) = 0;
    virtual bool _touch_up(TouchParam param) = 0;
    virtual bool _press_key(PressKeyParam param) = 0;
    virtual bool _input_text(InputTextParam param) = 0;

protected:
    MessageNotifier notifier_;

private:
    MaaCtrlId post_connection_impl();
    MaaCtrlId post_click_impl(int x, int y);
    MaaCtrlId post_swipe_impl(int x1, int y1, int x2, int y2, int duration);
    MaaCtrlId post_multi_swipe_impl(const std::vector<SwipeParam>& swipes);
    MaaCtrlId post_press_key_impl(int keycode);
    MaaCtrlId post_input_text_impl(const std::string& text);
    MaaCtrlId post_start_app_impl(const std::string& text);
    MaaCtrlId post_stop_app_impl(const std::string& text);
    MaaCtrlId post_screencap_impl();

    MaaCtrlId post_touch_down_impl(int contact, int x, int y, int pressure);
    MaaCtrlId post_touch_move_impl(int contact, int x, int y, int pressure);
    MaaCtrlId post_touch_up_impl(int contact);

    bool handle_connect();
    bool handle_click(const ClickParam& param);
    bool handle_swipe(const SwipeParam& param);
    bool handle_multi_swipe(const std::vector<SwipeParam>& param);
    bool handle_touch_down(const TouchParam& param);
    bool handle_touch_move(const TouchParam& param);
    bool handle_touch_up(const TouchParam& param);
    bool handle_press_key(const PressKeyParam& param);
    bool handle_input_text(const InputTextParam& param);
    bool handle_screencap();
    bool handle_start_app(const AppParam& param);
    bool handle_stop_app(const AppParam& param);

    bool recording() const;
    void init_recording();
    void append_recording(json::value info, const std::chrono::steady_clock::time_point& start_time, bool success);

    MaaCtrlId post(Action action);
    void focus_id(MaaCtrlId id);
    bool check_stop();

private:
    static cv::Point rand_point(const cv::Rect& r);

    bool run_action(typename AsyncRunner<Action>::Id id, Action action);
    std::pair<int, int> preproc_touch_point(int x, int y);
    bool postproc_screenshot(const cv::Mat& raw);
    bool calc_target_image_size();
    void clear_target_image_size();
    bool request_uuid();
    bool init_scale_info();

private: // options
    bool set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_image_use_raw_size(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_recording(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    bool need_to_stop_ = false;

private:
    static std::minstd_rand rand_engine_;

    bool connected_ = false;
    std::mutex image_mutex_;
    cv::Mat image_;

    bool image_use_raw_size_ = false;
    int image_target_long_side_ = 0;
    int image_target_short_side_ = 720;
    int image_target_width_ = 0;
    int image_target_height_ = 0;
    int image_raw_width_ = 0;
    int image_raw_height_ = 0;

    std::string uuid_cache_;

    bool recording_ = false;
    std::filesystem::path recording_path_;

    std::set<AsyncRunner<Action>::Id> focus_ids_;
    std::mutex focus_ids_mutex_;
    std::unique_ptr<AsyncRunner<Action>> action_runner_ = nullptr;
};

MAA_CTRL_NS_END
