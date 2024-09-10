#include "ProjectInterfaceMgr.h"

#include "CLI/interactor.h"
#include "Config/GlobalOptionConfig.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

void ProjectInterfaceMgr::register_custom_recognizer(const std::string& name, MAA_PROJECT_INTERFACE_NS::CustomRecognizerSession recognizer)
{
    custom_recognizers_.insert_or_assign(name, recognizer);
}

void ProjectInterfaceMgr::register_custom_action(const std::string& name, MAA_PROJECT_INTERFACE_NS::CustomActionSession action)
{
    custom_actions_.insert_or_assign(name, action);
}

bool ProjectInterfaceMgr::run_cli(
    const std::filesystem::path& resource_path,
    const std::filesystem::path& user_path,
    bool directly,
    MaaNotificationCallback callback,
    void* callback_arg)
{
    auto& config = MAA_TOOLKIT_NS::GlobalOptionConfig::get_instance();
    config.init(user_path, {});

    Interactor interactor;

    if (!interactor.load(resource_path, callback, callback_arg, custom_recognizers_, custom_actions_)) {
        return false;
    }
    if (directly) {
        interactor.print_config();
        return interactor.run();
    }

    interactor.interact();
    return true;
}

MAA_TOOLKIT_NS_END
