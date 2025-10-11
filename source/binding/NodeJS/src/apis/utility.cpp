#include <string>

#include <MaaFramework/MaaAPI.h>

std::string version_macro()
{
    return MAA_VERSION;
}

std::string version()
{
    return MaaVersion();
}
