#include "MaatouchController.h"

MAA_CTRL_NS_BEGIN

MaatouchController::MaatouchController(const std::filesystem::path& adb_path, const std::string& address,
                                       MaaControllerCallback callback, void* callback_arg)
    : AdbController(adb_path, address, callback, callback_arg)
{}

MaatouchController::~MaatouchController() {}

void MaatouchController::_click(ClickParams param) {}

void MaatouchController::_swipe(SwipeParams param) {}

MAA_CTRL_NS_END
