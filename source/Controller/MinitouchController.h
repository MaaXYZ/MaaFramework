#pragma once
#include "AdbController.h"

MAA_CTRL_NS_BEGIN

class MinitouchController : public AdbController
{
public:
    MinitouchController(const std::filesystem::path& adb_path, const std::string& address,
                        MaaControllerCallback callback, void* callback_arg);
    virtual ~MinitouchController() override;

protected:
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
};

MAA_CTRL_NS_END
