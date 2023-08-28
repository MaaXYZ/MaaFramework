#pragma once

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "Base/MessageNotifier.hpp"
#include "Instance/InstanceInternalAPI.hpp"
#include "Utils/NoWarningCVMat.hpp"

#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <variant>

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
    struct Step
    {
        int x = 0;
        int y = 0;
        int delay = 0;
    };
    std::vector<Step> steps;
};
struct PressKeyParam
{
    int keycode = 0;
};
struct AppParam
{
    std::string package;
};

std::ostream& operator<<(std::ostream& os, const SwipeParam::Step& step);

using Param = std::variant<std::monostate, ClickParam, SwipeParam, PressKeyParam, AppParam>;

struct Action
{
    enum class Type
    {
        connect,
        click,
        swipe,
        press_key,
        screencap,
        start_app,
        stop_app,
    } type;

    Param param;
};

std::ostream& operator<<(std::ostream& os, const Action& action);

class ControllerMgr : public MaaControllerAPI
{
public:
    ControllerMgr(MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    virtual ~ControllerMgr() override;

    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaCtrlId post_connection() override;
    virtual MaaCtrlId post_click(int x, int y) override;
    virtual MaaCtrlId post_swipe(std::vector<int> x_steps, std::vector<int> y_steps,
                                 std::vector<int> step_delay) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const override;
    virtual MaaBool connected() const override;

    virtual std::vector<uint8_t> get_image_cache() const override;
    virtual std::string get_uuid() const override = 0;

    virtual void on_stop() override;

public:
    void click(const cv::Rect& r);
    void click(const cv::Point& p);
    void swipe(const cv::Rect& r1, const cv::Rect& r2, int duration);
    void swipe(const cv::Point& p1, const cv::Point& p2, int duration);
    void press_key(int keycode);
    cv::Mat screencap();

    void start_app();
    void stop_app();
    void start_app(const std::string& package);
    void stop_app(const std::string& package);

protected:
    virtual bool _connect() = 0;
    virtual std::pair<int, int> _get_resolution() const = 0;
    virtual void _click(ClickParam param) = 0;
    virtual void _swipe(SwipeParam param) = 0;
    virtual void _press_key(PressKeyParam param) = 0;
    virtual cv::Mat _screencap() = 0;
    virtual bool _start_app(AppParam param) = 0;
    virtual bool _stop_app(AppParam param) = 0;

protected:
    MessageNotifier<MaaControllerCallback> notifier;

private:
    static cv::Point rand_point(const cv::Rect& r);

    bool run_action(typename AsyncRunner<Action>::Id id, Action action);
    std::pair<int, int> preproc_touch_coord(int x, int y);
    bool postproc_screenshot(const cv::Mat& raw);
    bool check_and_calc_target_image_size(const cv::Mat& raw);
    void clear_target_image_size();

private: // options
    bool set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_default_app_package_entry(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_default_app_package(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    // InstanceInternalAPI* inst_ = nullptr;

private:
    static std::minstd_rand rand_engine_;

    bool connected_ = false;
    std::mutex image_mutex_;
    cv::Mat image_;

    int image_target_long_side_ = 0;
    int image_target_short_side_ = 720;
    int image_target_width_ = 0;
    int image_target_height_ = 0;

    std::string default_app_package_entry_;
    std::string default_app_package_;

    std::set<AsyncRunner<Action>::Id> post_ids_;
    std::mutex post_ids_mutex_;
    std::unique_ptr<AsyncRunner<Action>> action_runner_ = nullptr;
};

MAA_CTRL_NS_END
