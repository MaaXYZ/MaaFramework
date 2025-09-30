#include "MaaToolkit/Config/MaaToolkitConfig.h"

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>
#include <quickjs.h>

#include "Config/GlobalOptionConfig.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

MaaBool MaaToolkitConfigInitOption(const char* user_path, const char* default_json)
{
    LogInfo << VAR(user_path) << VAR(default_json);

    auto json_opt = json::parse(default_json);
    if (!json_opt) {
        LogError << "failed to parse json" << default_json;
        return false;
    }

    auto& config = MAA_TOOLKIT_NS::GlobalOptionConfig::get_instance();
    return config.init(MAA_NS::path(user_path), *json_opt);
}

const char* MaaToolkitQuickJSTest(const char* script)
{
    static std::string res;

    auto rt = JS_NewRuntime();
    auto ctx = JS_NewContext(rt);

    auto globalThis = JS_GetGlobalObject(ctx);
    JSCFunctionListEntry entries[] = {
        JS_CFUNC_DEF(
            "MaaVersion",
            0,
            +[](JSContext* ctx, JSValueConst, int, JSValueConst*) -> JSValue { return JS_NewString(ctx, MaaVersion()); }),
    };
    JS_SetPropertyFunctionList(ctx, globalThis, entries, 1);

    auto val = JS_Eval(ctx, script, strlen(script), "index.js", 0);
    res = JS_IsString(val) ? JS_ToCString(ctx, val) : "";

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    return res.c_str();
}
