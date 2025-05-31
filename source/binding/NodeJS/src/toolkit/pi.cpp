#include "../include/forward.h"

#include <MaaToolkit/MaaToolkitAPI.h>

#include "../include/cb.h"
#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

void pi_register_custom_recognizer(Napi::Env env, ExtContextInfo* context, uint64_t id, std::string name, Napi::Function callback)
{
    auto ctx = new CallbackContext(env, callback, "CustomRecognizerCallback");
    MaaToolkitProjectInterfaceRegisterCustomRecognition(id, name.c_str(), CustomRecognizerCallback, ctx);
    context->picli.emplace_back(ctx);
}

void pi_register_custom_action(Napi::Env env, ExtContextInfo* context, uint64_t id, std::string name, Napi::Function callback)
{
    auto ctx = new CallbackContext(env, callback, "CustomActionCallback");
    MaaToolkitProjectInterfaceRegisterCustomAction(id, name.c_str(), CustomActionCallback, ctx);
    context->picli.emplace_back(ctx);
}

Napi::Promise pi_run_cli(
    Napi::Env env,
    uint64_t id,
    std::string resource_path,
    std::string user_path,
    bool directly,
    std::optional<Napi::Function> callback)
{
    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    auto worker = new SimpleAsyncWork<bool, "pi_run_cli">(
        env,
        [id, resource_path, user_path, directly, cb, ctx]() {
            return MaaToolkitProjectInterfaceRunCli(id, resource_path.c_str(), user_path.c_str(), directly, cb, ctx);
        },
        [ctx](auto env, auto res) {
            delete ctx;
            return JSConvert<bool>::to_value(env, res);
        });
    worker->Queue();
    return worker->Promise();
}

void load_toolkit_pi(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(pi_register_custom_recognizer);
    BIND(pi_register_custom_action);
    BIND(pi_run_cli);
}
