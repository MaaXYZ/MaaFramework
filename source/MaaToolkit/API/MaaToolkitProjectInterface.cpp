#include "MaaToolkit/ProjectInterface/MaaToolkitProjectInterface.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ProjectInterface/ProjectInterfaceMgr.h"

MaaBool MaaToolkitProjectInterfaceLoad(const char* interface_path)
{
    LogInfo << VAR(interface_path);

    if (!interface_path) {
        LogError << "interface_path is null";
        return false;
    }

    auto& mgr = MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance();
    return mgr.load(MAA_NS::path(interface_path));
}

MaaBool MaaToolkitProjectInterfaceLoaded()
{
    return MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance().loaded();
}

MaaBool MaaToolkitProjectInterfaceBindResource(MaaResource* resource)
{
    LogFunc << VAR(resource);

    auto& mgr = MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance();
    return mgr.bind_resource(resource);
}

MaaBool MaaToolkitProjectInterfaceStartAgent()
{
    LogFunc;

    auto& mgr = MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance();
    return mgr.start_agent();
}

void MaaToolkitProjectInterfaceStopAgent()
{
    LogFunc;

    auto& mgr = MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance();
    mgr.stop_agent();
}

MaaBool MaaToolkitProjectInterfaceAgentRunning()
{
    auto& mgr = MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance();
    return mgr.agent_running();
}

MaaBool MaaToolkitProjectInterfaceAgentConnected()
{
    auto& mgr = MAA_TOOLKIT_NS::ProjectInterfaceMgr::get_instance();
    return mgr.agent_connected();
}
