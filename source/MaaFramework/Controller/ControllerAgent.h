#pragma once

#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <variant>

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "Instance/InstanceInternalAPI.hpp"
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
};

struct SwipeParam
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int duration = 0;
};

struct TouchParam
{
    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;
};

struct PressKeyParam
{
    int keycode = 0;
};

struct InputTextParam
{
    std::string text;
};

struct AppParam
{
    std::string package;
};

using Param = std::variant<
    std::monostate,
    ClickParam,
    SwipeParam,
    TouchParam,
    PressKeyParam,
    InputTextParam,
    AppParam>;

struct Action
{
    enum class Type
    {
        invalid,
        connect,
        click,
        swipe,
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

class ControllerAgent : public MaaControllerAPI
{
public:
    ControllerAgent(MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    virtual ~ControllerAgent() override;

    virtual bool
        set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaCtrlId post_connection() override;
    virtual MaaCtrlId post_click(int x, int y) override;
    virtual MaaCtrlId post_swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual MaaCtrlId post_press_key(int keycode) override;
    virtual MaaCtrlId post_input_text(std::string_view text) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_up(int contact) override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const override;
    virtual MaaBool connected() const override;
    virtual MaaBool running() const override;

    virtual cv::Mat get_image() override;
    virtual std::string get_uuid() override;
    virtual std::pair<int, int> get_resolution() override;

public:
    virtual void post_stop() override;

public:
    bool click(const cv::Rect& r);
    bool click(const cv::Point& p);
    bool swipe(const cv::Rect& r1, const cv::Rect& r2, int duration);
    bool swipe(const cv::Point& p1, const cv::Point& p2, int duration);
    bool press_key(int keycode);
    bool input_text(const std::string& text);
    cv::Mat screencap();

    bool start_app();
    bool stop_app();
    bool start_app(const std::string& package);
    bool stop_app(const std::string& package);

protected:
    virtual bool _connect() = 0;
    virtual std::optional<std::string> _request_uuid() = 0;
    virtual std::optional<std::pair<int, int>> _request_resolution() = 0;
    virtual bool _start_app(AppParam param) = 0;
    virtual bool _stop_app(AppParam param) = 0;
    virtual std::optional<cv::Mat> _screencap() = 0;
    virtual bool _click(ClickParam param) = 0;
    virtual bool _swipe(SwipeParam param) = 0;
    virtual bool _touch_down(TouchParam param) = 0;
    virtual bool _touch_move(TouchParam param) = 0;
    virtual bool _touch_up(TouchParam param) = 0;
    virtual bool _press_key(PressKeyParam param) = 0;
    virtual bool _input_text(InputTextParam param) = 0;

protected:
    MessageNotifier<MaaControllerCallback> notifier;

private:
    MaaCtrlId post_connection_impl();
    MaaCtrlId post_click_impl(int x, int y);
    MaaCtrlId post_swipe_impl(int x1, int y1, int x2, int y2, int duration);
    MaaCtrlId post_press_key_impl(int keycode);
    MaaCtrlId post_input_text_impl(std::string_view text);
    MaaCtrlId post_screencap_impl();

    MaaCtrlId post_touch_down_impl(int contact, int x, int y, int pressure);
    MaaCtrlId post_touch_move_impl(int contact, int x, int y, int pressure);
    MaaCtrlId post_touch_up_impl(int contact);

    bool handle_connect();
    bool handle_click(const ClickParam& param);
    bool handle_swipe(const SwipeParam& param);
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
    void append_recording(
        json::value info,
        const std::chrono::steady_clock::time_point& start_time,
        bool success);

    MaaCtrlId post(Action action);
    void focus_id(MaaCtrlId id);
    bool check_stop();

private:
    static cv::Point rand_point(const cv::Rect& r);

    bool run_action(typename AsyncRunner<Action>::Id id, Action action);
    std::pair<int, int> preproc_touch_point(int x, int y);
    bool postproc_screenshot(const cv::Mat& raw);
    bool check_and_calc_target_image_size(const cv::Mat& raw);
    void clear_target_image_size();
    bool request_uuid();
    bool request_resolution();

private: // options
    bool set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_default_app_package_entry(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_default_app_package(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_recording(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    bool need_to_stop_ = false;

private:
    static std::minstd_rand rand_engine_;

    bool connected_ = false;
    std::mutex image_mutex_;
    cv::Mat image_;

    int image_target_long_side_ = 0;
    int image_target_short_side_ = 720;
    int image_target_width_ = 0;
    int image_target_height_ = 0;

    std::string uuid_cache_;
    std::pair<int, int> resolution_cache_ = { 0, 0 };

    std::string default_app_package_entry_;
    std::string default_app_package_;

    bool recording_ = false;
    std::filesystem::path recording_path_;

    std::set<AsyncRunner<Action>::Id> focus_ids_;
    std::mutex focus_ids_mutex_;
    std::unique_ptr<AsyncRunner<Action>> action_runner_ = nullptr;
};

MAA_CTRL_NS_END
