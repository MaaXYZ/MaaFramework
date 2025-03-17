module;

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include <string>

#include "../include/macro.h"

export module maa.nodejs.toolkit.config;

import napi;

import maa.nodejs.info;
import maa.nodejs.wrapper;

bool config_init_option(std::string user_path, std::string default_json)
{
    return MaaToolkitConfigInitOption(user_path.c_str(), default_json.c_str());
}

export void load_toolkit_config(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(config_init_option);
}
