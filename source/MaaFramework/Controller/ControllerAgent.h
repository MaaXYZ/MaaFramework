#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <variant>

#include "Base/AsyncRunner.hpp"
#include "Common/MaaTypes.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Utils/EventDispatcher.hpp"

#include "Common/Conf.h"

MAA_RES_NS_BEGIN
class ResourceMgr;
MAA_RES_NS_END

MAA_CTRL_NS_BEGIN

struct ClickParam
{
    cv::Point point {};
    int contact = 0;

    MEO_TOJSON(point, contact);
};

struct LongPressParam
{
    cv::Point point {};
    uint duration = 0;
    int contact = 0;

    MEO_TOJSON(point, duration, contact);
};

struct SwipeParam
{
    cv::Point begin {};
    std::vector<cv::Point> end;
    std::vector<uint> end_hold;
    std::vector<uint> duration;
    bool only_hover = false;
    uint starting = 0;
    int contact = 0;

    MEO_TOJSON(starting, begin, end, end_hold, duration, only_hover, contact);
};

struct MultiSwipeParam
{
    std::vector<SwipeParam> swipes;

    MEO_TOJSON(swipes);
};

struct TouchParam
{
    int contact = 0;
    cv::Point point {};
    int pressure = 0;

    MEO_TOJSON(contact, point, pressure);
};

struct ClickKeyParam
{
    std::vector<int> keycode;

    MEO_TOJSON(keycode);
};

struct LongPressKeyParam
{
    std::vector<int> keycode;
    uint duration = 0;

    MEO_TOJSON(keycode, duration);
};

struct InputTextParam
{
    std::string text;

    MEO_TOJSON(text);
};

struct AppParam
{
    std::string package;

    MEO_TOJSON(package);
};

struct ScrollParam
{
    int dx = 0;
    int dy = 0;

    MEO_TOJSON(dx, dy);
};

struct ShellParam
{
    std::string cmd;
    int64_t timeout = 20000;

    MEO_TOJSON(cmd, timeout);
};

using Param = std::variant<
    std::monostate,
    ClickParam,
    LongPressParam,
    SwipeParam,
    MultiSwipeParam,
    TouchParam,
    ClickKeyParam,
    LongPressKeyParam,
    InputTextParam,
    AppParam,
    ScrollParam,
    ShellParam>;

struct Action
{
    enum class Type
    {
        invalid,
        connect,
        click,
        long_press,
        swipe,
        multi_swipe,
        touch_down,
        touch_move,
        touch_up,
        click_key,
        long_press_key,
        input_text,
        screencap,
        start_app,
        stop_app,
        key_down,
        key_up,
        scroll,
        shell,
    } type = Type::invalid;

    Param param;
};

class ControllerAgent : public MaaController
{
public:
    ControllerAgent(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit);
    virtual ~ControllerAgent() override;

public: // MaaController
    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaCtrlId post_connection() override;
    virtual MaaCtrlId post_click(int x, int y) override;
    virtual MaaCtrlId post_swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual MaaCtrlId post_click_key(int keycode) override;
    virtual MaaCtrlId post_input_text(const std::string& text) override;
    virtual MaaCtrlId post_start_app(const std::string& intent) override;
    virtual MaaCtrlId post_stop_app(const std::string& intent) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_up(int contact) override;

    virtual MaaCtrlId post_key_down(int keycode) override;
    virtual MaaCtrlId post_key_up(int keycode) override;

    virtual MaaCtrlId post_scroll(int dx, int dy) override;

    virtual MaaCtrlId post_shell(const std::string& cmd, int64_t timeout = 20000) override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const override;
    virtual bool connected() const override;
    virtual bool running() const override;

    virtual cv::Mat cached_image() const override;
    virtual std::string cached_shell_output() const override;
    virtual std::string get_uuid() override;

    virtual bool get_resolution(int32_t& width, int32_t& height) const override;

    virtual MaaSinkId add_sink(MaaEventCallback callback, void* trans_arg) override;
    virtual void remove_sink(MaaSinkId sink_id) override;
    virtual void clear_sinks() override;

public: // for Actuator
    void post_stop();

    bool click(ClickParam p);
    bool long_press(LongPressParam p);

    bool swipe(SwipeParam p);
    bool multi_swipe(MultiSwipeParam p);

    bool touch_down(TouchParam p);
    bool touch_move(TouchParam p);
    bool touch_up(TouchParam p);

    bool click_key(ClickKeyParam p);
    bool long_press_key(LongPressKeyParam p);
    bool key_down(ClickKeyParam p);
    bool key_up(ClickKeyParam p);

    bool input_text(InputTextParam p);
    cv::Mat screencap();

    bool start_app(AppParam p);
    bool stop_app(AppParam p);

    bool scroll(ScrollParam p);
    bool shell(const std::string& cmd, std::string& output, int64_t timeout = 20000);

private:
    bool handle_connect();
    bool handle_click(const ClickParam& param);
    bool handle_long_press(const LongPressParam& param);
    bool handle_swipe(const SwipeParam& param);
    bool handle_multi_swipe(const MultiSwipeParam& param);
    bool handle_touch_down(const TouchParam& param);
    bool handle_touch_move(const TouchParam& param);
    bool handle_touch_up(const TouchParam& param);
    bool handle_click_key(const ClickKeyParam& param);
    bool handle_long_press_key(const LongPressKeyParam& param);
    bool handle_input_text(const InputTextParam& param);
    bool handle_screencap();
    bool handle_start_app(const AppParam& param);
    bool handle_stop_app(const AppParam& param);
    bool handle_key_down(const ClickKeyParam& param);
    bool handle_key_up(const ClickKeyParam& param);
    bool handle_scroll(const ScrollParam& param);
    bool handle_shell(const ShellParam& param);

    MaaCtrlId post(Action action);
    MaaCtrlId focus_id(MaaCtrlId id);
    bool check_stop();

private:
    bool run_action(typename AsyncRunner<Action>::Id id, Action action);
    cv::Point preproc_touch_point(const cv::Point& p);
    bool postproc_screenshot(const cv::Mat& raw);
    bool calc_target_image_size();
    void clear_target_image_size();
    bool request_uuid();
    bool init_scale_info();

private: // options
    bool set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_image_use_raw_size(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    bool need_to_stop_ = false;

private:
    const std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit_ = nullptr;
    EventDispatcher notifier_;

    bool connected_ = false;
    mutable std::mutex image_mutex_;
    cv::Mat image_;
    mutable std::mutex shell_output_mutex_;
    std::string shell_output_;

    bool image_use_raw_size_ = false;
    int image_target_long_side_ = 0;
    int image_target_short_side_ = 720;
    int image_target_width_ = 0;
    int image_target_height_ = 0;
    int image_raw_width_ = 0;
    int image_raw_height_ = 0;

    std::string uuid_cache_;

    std::set<AsyncRunner<Action>::Id> focus_ids_;
    std::mutex focus_ids_mutex_;
    std::unique_ptr<AsyncRunner<Action>> action_runner_ = nullptr;
};

MAA_CTRL_NS_END
