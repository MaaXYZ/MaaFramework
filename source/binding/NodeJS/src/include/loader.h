#include <napi.h>

struct ExtContextInfo;

void load_instance_context(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
void load_instance_controller(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
void load_instance_resource(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
void load_instance_tasker(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);

void load_utility_utility(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);

void load_toolkit_config(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
void load_toolkit_find(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
void load_toolkit_pi(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);

void load_toolkit_find(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
void load_toolkit_pi(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);

void load_agent(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
