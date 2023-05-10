#pragma once

#include <mutex>
#include <variant>

#include "MaaDef.h"
#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

#include "Utils/AsyncRunner.hpp"
#include "Utils/NoWarningCVMat.h"

MAA_CTRL_NS_BEGIN

class ControllerMgr : public MaaControllerAPI
{
public:
    virtual MaaCtrlId click(int x, int y) override = 0;
    virtual MaaCtrlId swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                            const std::vector<int>& step_delay) override = 0;
    virtual MaaCtrlId screencap() override = 0;

public:
    ControllerMgr(MaaControllerCallback callback, void* callback_arg);

    virtual ~ControllerMgr() override;

    virtual bool set_option(std::string_view key, std::string_view value) override;

    virtual bool connecting() const override;
    virtual bool connected() const override;

    virtual std::vector<unsigned char> get_image() const override;
    virtual std::string get_uuid() const override;

protected:
    virtual bool do_connect(const std::string& adb_path, const std::string& address, const std::string& config) = 0;
    virtual bool do_click(int x, int y) = 0;
    virtual bool do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                            const std::vector<int>& step_delay) = 0;
    virtual bool do_screencap() = 0;

    MaaControllerCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;

    struct CallItem
    {
        enum Type
        {
            Connect,
            Click,
            Screencap
        };

        struct ConnectParams
        {
            std::string adb_path;
            std::string address;
            std::string config;
        };
        struct ClickParams
        {
            int x = 0;
            int y = 0;
        };
        struct ScreencapParams
        {};

        Type type;
        std::variant<ConnectParams, ClickParams, ScreencapParams> param;
    };

    AsyncRunner<CallItem, MaaCtrlId>* runner_;

private:
    void runnerDispatcher(MaaCtrlId id, CallItem item);

    std::mutex cache_image_mutex_;
    cv::Mat cache_image_;
};

MAA_CTRL_NS_END
