#pragma once

#include "Base/AsyncCallback.hpp"
#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

#include "Utils/NoWarningCVMat.h"

#include <memory>
#include <mutex>
#include <random>
#include <variant>

MAA_CTRL_NS_BEGIN

struct ClickParams
{
    int x = 0;
    int y = 0;
};
struct SwipeParams
{
    struct Step
    {
        int x = 0;
        int y = 0;
        int delay = 0;
    };
    std::vector<Step> steps;
};

using Params = std::variant<ClickParams, SwipeParams>;

struct Action
{
    enum class Type
    {
        connect,
        click,
        swipe,
        screencap,
    } type;

    Params params;
};

std::ostream& operator<<(std::ostream& os, const Action& action);

class ControllerMgr : public MaaControllerAPI
{
public:
    ControllerMgr(MaaControllerCallback callback, void* callback_arg);

    virtual ~ControllerMgr() override;

    virtual bool set_option(std::string_view key, std::string_view value) override;

    virtual MaaCtrlId post_connect() override;
    virtual MaaCtrlId post_click(int x, int y) override;
    virtual MaaCtrlId post_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                                 const std::vector<int>& step_delay) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaBool connected() const override;

    virtual std::vector<uint8_t> get_image() const override;
    virtual std::string get_uuid() const override;

public:
    void click(const cv::Rect& r);
    void click(const cv::Point& p);
    void swipe(const cv::Rect& r1, const cv::Rect& r2, int duration);
    void swipe(const cv::Point& p1, const cv::Point& p2, int duration);
    cv::Mat screencap();

protected:
    virtual bool _connect() = 0;
    virtual void _click(ClickParams param) = 0;
    virtual void _swipe(SwipeParams param) = 0;
    virtual cv::Mat _screencap() = 0;

private:
    bool run_action(typename AsyncRunner<Action>::Id id, Action action);
    cv::Point rand_point(const cv::Rect& r);

private:
    std::minstd_rand rand_engine_;

    bool connected_ = false;
    std::mutex image_mutex_;
    cv::Mat image_;

    std::unique_ptr<AsyncRunner<Action>> action_runner_ = nullptr;
    AsyncCallback<MaaControllerCallback, void*> notifier;
};

MAA_CTRL_NS_END
