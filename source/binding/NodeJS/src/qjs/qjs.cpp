#include "qjs.hpp"

#include <quickjs.h>

namespace qjs
{

void* RuntimeRef::GetOpaque()
{
    return JS_GetRuntimeOpaque(runtime_);
}

void RuntimeRef::SetOpaque(void* opaque)
{
    JS_SetRuntimeOpaque(runtime_, opaque);
}

void RuntimeRef::AddFinalizer(std::function<void()> func)
{
    auto pFunc = new std::function<void()>(std::move(func));
    JS_AddRuntimeFinalizer(
        runtime_,
        +[](JSRuntime*, void* arg) {
            auto pFunc = static_cast<std::function<void()>*>(arg);
            (*pFunc)();
            delete pFunc;
        },
        pFunc);
}

Runtime::Runtime()
{
    runtime_ = JS_NewRuntime();
}

Runtime::Runtime(Runtime&& rt)
{
    runtime_ = rt.runtime_;
    rt.runtime_ = nullptr;
}

Runtime::~Runtime()
{
    if (runtime_) {
        JS_FreeRuntime(runtime_);
    }
}

Runtime& Runtime::operator=(Runtime&& rt)
{
    if (this == &rt) {
        return *this;
    }
    this->~Runtime();
    runtime_ = rt.runtime_;
    rt.runtime_ = nullptr;
    return *this;
}

Context::Context(RuntimeRef rt)
{
    context_ = JS_NewContext(rt.runtime_);
}

Context::Context(const Context& ctx)
{
    context_ = ctx.context_ ? JS_DupContext(ctx.context_) : nullptr;
}

Context::Context(Context&& ctx)
{
    context_ = ctx.context_;
    ctx.context_ = nullptr;
}

Context::~Context()
{
    if (context_) {
        JS_FreeContext(context_);
    }
}

Context& Context::operator=(const Context& ctx)
{
    if (this == &ctx) {
        return *this;
    }
    this->~Context();
    context_ = ctx.context_ ? JS_DupContext(ctx.context_) : nullptr;
    return *this;
}

Context& Context::operator=(Context&& ctx)
{
    if (this == &ctx) {
        return *this;
    }
    this->~Context();
    context_ = ctx.context_;
    ctx.context_ = nullptr;
    return *this;
}

void* Context::GetOpaque()
{
    return JS_GetContextOpaque(context_);
}

void Context::SetOpaque(void* opaque)
{
    JS_SetContextOpaque(context_, opaque);
}

RuntimeRef Context::GetRuntime()
{
    return {
        JS_GetRuntime(context_),
    };
}

Value Context::Eval(std::string input, std::string filename, int evalFlags)
{
    return Value {
        *this,
        JS_Eval(context_, input.c_str(), input.size(), filename.c_str(), evalFlags),
    };
}

Value Context::Null()
{
    return Value { *this, JS_NULL };
}

Value Context::Undefined()
{
    return Value { *this, JS_UNDEFINED };
}

Value Context::Exception()
{
    return Value { *this, JS_EXCEPTION };
}

Value Context::Nan()
{
    return Value { *this, JS_NAN };
}

Value::Value(Context ctx, const JSValue& val)
{
    context_ = ctx;
    value_ = new JSValue { val };
}

Value::Value(const Value& val)
{
    context_ = val.context_;
    value_ = val.value_ ? new JSValue { *val.value_ } : nullptr;
}

Value::Value(Value&& val)
{
    context_ = std::move(val.context_);
    value_ = val.value_;
    val.value_ = nullptr;
}

Value::~Value()
{
    if (value_) {
        delete value_;
    }
}

Value& Value::operator=(const Value& val)
{
    if (this == &val) {
        return *this;
    }
    this->~Value();
    context_ = val.context_;
    value_ = val.value_ ? new JSValue { *val.value_ } : nullptr;
    return *this;
}

Value& Value::operator=(Value&& val)
{
    if (this == &val) {
        return *this;
    }
    this->~Value();
    context_ = std::move(val.context_);
    value_ = val.value_;
    val.value_ = nullptr;
    return *this;
}

void Value::Free()
{
    if (value_ && context_) {
        JS_FreeValue(context_, *value_);
        delete value_;
        value_ = nullptr;
    }
}

bool Value::IsNumber()
{
    return JS_IsNumber(*value_);
}

bool Value::IsBool()
{
    return JS_IsBool(*value_);
}

bool Value::IsNull()
{
    return JS_IsNull(*value_);
}

bool Value::IsUndefined()
{
    return JS_IsUndefined(*value_);
}

bool Value::IsException()
{
    return JS_IsException(*value_);
}

bool Value::IsBigInt()
{
    return JS_IsBigInt(context_, *value_);
}

bool Value::IsString()
{
    return JS_IsString(*value_);
}

bool Value::IsSymbol()
{
    return JS_IsSymbol(*value_);
}

bool Value::IsObject()
{
    return JS_IsObject(*value_);
}

bool Value::IsNan()
{
    return JS_VALUE_IS_NAN(*value_);
}

Number::Number(Context ctx, int32_t val)
    : Value(ctx, JS_NewInt32(ctx, val))
{
}

Number::Number(Context ctx, double val)
    : Value(ctx, JS_NewFloat64(ctx, val))
{
}

int32_t Number::Int32Value()
{
    return JS_VALUE_GET_INT(*value_);
}

double Number::DoubleValue()
{
    return JS_VALUE_GET_FLOAT64(*value_);
}

Bool::Bool(Context ctx, bool val)
    : Value(ctx, JS_NewBool(ctx, val))
{
}

bool Bool::BoolValue()
{
    return JS_VALUE_GET_BOOL(*value_);
}

}
