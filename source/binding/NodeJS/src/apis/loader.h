#pragma once

#include <map>
#include <string>

#include "../foundation/spec.h"

maajs::ObjectType load_global(maajs::EnvType env);

maajs::ValueType load_job(maajs::EnvType env);

maajs::ValueType load_resource(maajs::EnvType env);

maajs::ValueType load_image_job(maajs::EnvType env);
maajs::ValueType load_controller(maajs::EnvType env);
maajs::ValueType load_adb_controller(maajs::EnvType env);
maajs::ValueType load_win32_controller(maajs::EnvType env);
#ifdef __APPLE__
maajs::ValueType load_playcover_controller(maajs::EnvType env);
#endif
maajs::ValueType load_dbg_controller(maajs::EnvType env);
maajs::ValueType load_custom_controller(maajs::EnvType env);

maajs::ValueType load_task_job(maajs::EnvType env);
maajs::ValueType load_tasker(maajs::EnvType env);

maajs::ValueType load_context(maajs::EnvType env);

maajs::ValueType load_client(maajs::EnvType env);
maajs::ValueType load_server(maajs::EnvType env);
maajs::ValueType load_plugin(maajs::EnvType env);

std::map<std::string, maajs::ValueType> load_constant(maajs::EnvType env);

#ifdef MAA_JS_IMPL_IS_QUICKJS
void init_module_maa(JSContext* ctx);
void init_module_sys(JSContext* ctx);
#endif
