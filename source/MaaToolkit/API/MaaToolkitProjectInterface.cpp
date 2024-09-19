#include "MaaToolkit/ProjectInterface/MaaToolkitProjectInterface.h"

#include "ProjectInterface/ProjectInterfaceMgr.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

void MaaToolkitProjectInterfaceRegisterCustomRecognition(
    uint64_t inst_id,
    const char* name,
    MaaCustomRecognitionCallback recognition,
    void* trans_arg)
{
    MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance().register_custom_recognition(
        inst_id,
        name,
        { .recognition = recognition, .trans_arg = trans_arg });
}

void MaaToolkitProjectInterfaceRegisterCustomAction(uint64_t inst_id, const char* name, MaaCustomActionCallback action, void* trans_arg)
{
    MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance().register_custom_action(inst_id, name, { .action = action, .trans_arg = trans_arg });
}

MaaBool MaaToolkitProjectInterfaceRunCli(
    uint64_t inst_id,
    const char* resource_path,
    const char* user_path,
    MaaBool directly,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
{
    return MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance()
        .run_cli(inst_id, MAA_NS::path(resource_path), MAA_NS::path(user_path), directly, notify, notify_trans_arg);
}
