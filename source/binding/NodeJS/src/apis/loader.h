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

maajs::ValueType load_task_job(maajs::EnvType env);
maajs::ValueType load_tasker(maajs::EnvType env);

maajs::ValueType load_context(maajs::EnvType env);

std::map<std::string, maajs::ValueType> load_constant(maajs::EnvType env);
