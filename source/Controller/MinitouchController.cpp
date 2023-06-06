#include "MinitouchController.h"

MAA_CTRL_NS_BEGIN

MinitouchController::MinitouchController(const std::filesystem::path& adb_path, const std::string& address,
                                         MaaControllerCallback callback, void* callback_arg)
    : AdbController(adb_path_, address, callback, callback_arg)
{}

MinitouchController::~MinitouchController() {}

void MinitouchController::_click(ClickParams param) {}

void MinitouchController::_swipe(SwipeParams param) {}

MAA_CTRL_NS_END
