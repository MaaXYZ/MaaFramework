#pragma once

#include <cstdint>
#include <functional>
#include <string>

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;
typedef struct JSObject JSObject;
typedef struct JSClass JSClass;
typedef struct JSValue JSValue;
typedef std::uint32_t JSClassID;
typedef std::uint32_t JSAtom;

namespace qjs
{

struct Value;

struct RuntimeRef
{
    void* GetOpaque();
    void SetOpaque(void* opaque);
    void AddFinalizer(std::function<void()> func);

    JSRuntime* runtime_ = nullptr;
};

struct Runtime : public RuntimeRef
{
    Runtime();
    Runtime(const Runtime&) = delete;
    Runtime(Runtime&& rt);
    ~Runtime();
    Runtime& operator=(const Runtime&) = delete;
    Runtime& operator=(Runtime&& rt);
};

struct Context
{
    Context() = default;
    Context(RuntimeRef rt);
    Context(const Context& ctx);
    Context(Context&& ctx);
    ~Context();
    Context& operator=(const Context& ctx);
    Context& operator=(Context&& ctx);

    operator JSContext*() const { return context_; }

    void* GetOpaque();
    void SetOpaque(void* opaque);
    RuntimeRef GetRuntime();

    Value Eval(std::string input, std::string filename, int evalFlags);

    Value Null();
    Value Undefined();
    Value Exception();
    Value Nan();

    JSContext* context_ = nullptr;
};

struct Value
{
    Value() = default;
    Value(Context ctx, const JSValue& val);
    Value(const Value& val);
    Value(Value&& val);
    ~Value();
    Value& operator=(const Value& val);
    Value& operator=(Value&& val);

    void Free();

    bool IsNumber();
    bool IsBool();
    bool IsNull();
    bool IsUndefined();
    bool IsException();
    bool IsBigInt();
    bool IsString();
    bool IsSymbol();
    bool IsObject();
    bool IsNan();

    Context context_;
    JSValue* value_ = nullptr;
};

struct Number : public Value
{
    Number(Context ctx, int32_t val);
    Number(Context ctx, double val);

    int32_t Int32Value();
    double DoubleValue();
};

struct Bool : public Value
{
    Bool(Context ctx, bool val);

    bool BoolValue();
};

}
