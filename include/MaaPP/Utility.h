#pragma once

#include <string_view>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

namespace maapp
{

inline std::string_view version()
{
    return MaaVersion();
}

}
