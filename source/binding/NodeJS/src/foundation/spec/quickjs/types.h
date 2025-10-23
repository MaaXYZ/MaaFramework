#pragma once

#include "wrapper.h" // IWYU pragma: export

namespace maajs
{

using EnvType = QjsEnv;
using ValueType = QjsValue;
using ObjectType = QjsObject;
using BooleanType = QjsBoolean;
using StringType = QjsString;
using NumberType = QjsNumber;
using FunctionType = QjsFunction;
using ArrayType = QjsArray;
using PromiseType = QjsPromise;
using ArrayBufferType = QjsArrayBuffer;

using ObjectRefType = QjsRef<ObjectType>;
using FunctionRefType = QjsRef<FunctionType>;
using WeakObjectRefType = QjsWeakRef<ObjectType>;

using CallbackInfo = QjsCallbackInfo;

}
