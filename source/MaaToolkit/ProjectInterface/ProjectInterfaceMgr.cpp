#include "ProjectInterfaceMgr.h"

#include "CLI/interactor.h"
#include "Config/GlobalOptionConfig.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

void ProjectInterfaceMgr::register_custom_recognition(
    uint64_t inst_id,
    const std::string& name,
    MAA_PROJECT_INTERFACE_NS::CustomRecognitionSession recognition)
{
    custom_recognitions_[inst_id].insert_or_assign(name, recognition);
}

void ProjectInterfaceMgr::register_custom_action(
    uint64_t inst_id,
    const std::string& name,
    MAA_PROJECT_INTERFACE_NS::CustomActionSession action)
{
    custom_actions_[inst_id].insert_or_assign(name, action);
}

bool ProjectInterfaceMgr::run_cli(
    uint64_t inst_id,
    const std::filesystem::path& resource_path,
    const std::filesystem::path& user_path,
    bool directly,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
{
    auto& config = MAA_TOOLKIT_NS::GlobalOptionConfig::get_instance();
    config.init(user_path, {});

    Interactor interactor(user_path, notify, notify_trans_arg, custom_recognitions_[inst_id], custom_actions_[inst_id]);

    if (!interactor.load(resource_path)) {
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
