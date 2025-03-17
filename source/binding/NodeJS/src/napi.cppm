module;

#include <napi.h>

export module napi;

export namespace Napi
{

using Napi::Array;
using Napi::ArrayBuffer;
using Napi::AsyncWorker;
using Napi::Boolean;
using Napi::Buffer;
using Napi::CallbackInfo;
using Napi::Env;
using Napi::Error;
using Napi::External;
using Napi::Function;
using Napi::Number;
using Napi::Object;
using Napi::Promise;
using Napi::Reference;
using Napi::String;
using Napi::ThreadSafeFunction;
using Napi::TypeError;
using Napi::Value;

using Napi::Persistent;
using Napi::Weak;

}
