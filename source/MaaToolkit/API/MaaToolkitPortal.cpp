#include "MaaToolkit/Portal/MaaToolkitPortal.h"

#include "Portal/PortalHelper.h"

#include "MaaUtils/Logger.h"

MaaToolkitPortalHelper* MaaToolkitPortalHelperCreate()
{
#if defined(__linux__) && !defined(ANDROID)
    return new MaaToolkitPortalHelper;
#else
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return nullptr;
#endif
}

void MaaToolkitPortalHelperDestroy(MaaToolkitPortalHelper* helper)
{
#if defined(__linux__) && !defined(ANDROID)
    if (helper) {
        helper->close_internal();
        delete helper;
    }
#else
    std::ignore = helper;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return;
#endif
}

bool MaaToolkitPortalHelperOpenStream(MaaToolkitPortalHelper* helper)
{
#if defined(__linux__) && !defined(ANDROID)
    if (!helper) {
        LogError << "helper is null";
        return false;
    }

    if (!helper->dbus_create_session() || !helper->dbus_select_sources() || !helper->dbus_start_stream()) {
        // dbus_* methods already logged errors
        helper->close_internal();
        return false;
    }

    return true;
#else
    std::ignore = helper;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return false;
#endif
}

void MaaToolkitPortalHelperSetPersist(MaaToolkitPortalHelper* helper, MaaBool enable)
{
#if defined(__linux__) && !defined(ANDROID)
    if (!helper) {
        LogError << "helper is null";
        return;
    }

    helper->set_persist(enable);
#else
    std::ignore = helper;
    std::ignore = enable;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return false;
#endif
}

int MaaToolkitPortalHelperGetPipeWireFD(MaaToolkitPortalHelper* helper)
{
#if defined(__linux__) && !defined(ANDROID)
    if (!helper) {
        LogError << "helper is null";
        return -1;
    }
    return helper->get_fd();
#else
    std::ignore = helper;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return -1;
#endif
}

uint32_t MaaToolkitPortalHelperGetPipeWireNodeID(MaaToolkitPortalHelper* helper)
{
#if defined(__linux__) && !defined(ANDROID)
    if (!helper) {
        LogError << "helper is null";
        return 0;
    }
    return helper->get_node_id();
#else
    std::ignore = helper;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return 0;
#endif
}

const char* MaaToolkitPortalHelperGetRestoreToken(MaaToolkitPortalHelper* helper)
{
#if defined(__linux__) && !defined(ANDROID)
    if (!helper) {
        LogError << "helper is null";
        return nullptr;
    }
    return helper->get_restore_token();
#else
    std::ignore = helper;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return 0;
#endif
}

void MaaToolkitPortalHelperSetRestoreToken(MaaToolkitPortalHelper* helper, const char* token)
{
#if defined(__linux__) && !defined(ANDROID)
    if (!helper) {
        LogError << "helper is null";
        return;
    }

    if (!token) {
        LogError << "token is null";
        return;
    }

    helper->set_restore_token(token);
#else
    std::ignore = helper;
    std::ignore = token;
    LogError << "This API " << __FUNCTION__ << " is only available on Linux";
    return false;
#endif
}
