#pragma once

#include <napi.h> // IWYU pragma: export

namespace maajs
{

using EnvType = Napi::Env;
using ValueType = Napi::Value;
using ObjectType = Napi::Object;
using BooleanType = Napi::Boolean;
using StringType = Napi::String;
using NumberType = Napi::Number;
using FunctionType = Napi::Function;
using ArrayType = Napi::Array;
using PromiseType = Napi::Promise;
using ArrayBufferType = Napi::ArrayBuffer;

using ObjectRefType = Napi::ObjectReference;
using FunctionRefType = Napi::FunctionReference;
using WeakObjectRefType = Napi::ObjectReference;

using CallbackInfo = Napi::CallbackInfo;

}
