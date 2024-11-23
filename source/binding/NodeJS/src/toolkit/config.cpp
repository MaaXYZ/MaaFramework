#include "../include/loader.h"
#include "../include/wrapper.h"

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

bool config_init_option(std::string user_path, std::string default_json)
{
    return MaaToolkitConfigInitOption(user_path.c_str(), default_json.c_str());
}

void load_toolkit_config(
    Napi::Env env,
    Napi::Object& exports,
    Napi::External<ExtContextInfo> context)
{
    BIND(config_init_option);
}
