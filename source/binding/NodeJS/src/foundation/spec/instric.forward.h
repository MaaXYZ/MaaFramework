#pragma once

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "types.h"

namespace maajs
{

inline ValueType MakeArray(EnvType env, std::vector<ValueType> vals);
inline std::vector<ValueType> GetArray(const ArrayType& val);
inline ObjectRefType PersistentObject(ObjectType val);
inline FunctionRefType PersistentFunction(FunctionType val);
inline WeakObjectRefType WeakRefObject(ObjectType val);
inline ValueType ThrowTypeError(EnvType env, const std::string& err);
inline FunctionType
    MakeFunction(EnvType env, const char* name, int argc, RawCallback func, std::function<void(NativeMarkerFunc)> run_marker = nullptr);
inline FunctionType MakeFunction(EnvType env, const char* name, int argc, RawCallback func, std::shared_ptr<ObjectRefType> capture);

inline std::tuple<PromiseType, std::shared_ptr<FunctionRefType>, std::shared_ptr<FunctionRefType>> MakePromise(EnvType env);

inline void BindValue(ObjectType object, const char* prop, ValueType value);
inline void BindGetterSetter(
    ObjectType object,
    const char* prop,
    const char* getter_name,
    const char* setter_name,
    RawCallback getter,
    RawCallback setter,
    std::function<void(NativeMarkerFunc)> run_getter_marker = nullptr,
    std::function<void(NativeMarkerFunc)> run_setter_marker = nullptr);

inline ObjectType CallCtor(FunctionType ctor, std::vector<ValueType> args);

inline bool IsError(ValueType val);
inline std::string ClassName(ObjectType val);
inline std::string TypeOf(ValueType val);
inline std::string DumpValue(ValueType val);

inline void init(EnvType env);

}
