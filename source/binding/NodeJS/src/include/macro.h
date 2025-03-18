#pragma once

#define BIND(name) exports[#name] = Napi::Function::New(env, JSWrapFunction<decltype(&name), &name, #name>::make(context), #name)
