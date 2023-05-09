#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

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

    virtual bool set_option(ControllerOptionKey key, const std::string& value) override;

    virtual bool connecting() const override;
    virtual bool connected() const override;

    virtual std::vector<unsigned char> get_image() const override;
    virtual std::string get_uuid() const override;

protected:
    MaaControllerCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;
};

MAA_CTRL_NS_END
