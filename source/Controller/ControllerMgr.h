#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

MAA_CTRL_NS_BEGIN

class ControllerMgr : public MaaControllerAPI
{
public:
    struct Config
    {
        Config() = default;
        Config(const std::string& config_json)
        {
            // TODO
        }
    };

public:
    ControllerMgr(const std::filesystem::path& adb_path, const std::string& address, const std::string& config_json,
                  MaaControllerCallback callback, void* callback_arg);

    virtual ~ControllerMgr() override;

    virtual bool set_option(ControllerOptionKey key, const std::string& value) override;

    virtual bool connecting() const override;
    virtual bool connected() const override;

    virtual MaaCtrlId click(int x, int y) override;
    virtual MaaCtrlId swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                            const std::vector<int>& step_delay) override;
    virtual MaaCtrlId screencap() override;
    virtual std::vector<unsigned char> get_image() const override;

    virtual std::string get_uuid() const override;

protected:
    std::filesystem::path adb_path_;
    std::string address_;
    Config config_;
    MaaControllerCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;
};

MAA_CTRL_NS_END
