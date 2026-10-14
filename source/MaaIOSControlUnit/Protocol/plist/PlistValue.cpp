#include "plist/Plist.h"
#include <cmath>

namespace maa::ios::plist
{

Value Value::Bool(bool b)
{
    Value v;
    v.kind = Kind::Bool;
    v.boolean = b;
    return v;
}

Value Value::Int(int64_t i)
{
    Value v;
    v.kind = Kind::Int;
    v.integer = i;
    return v;
}

Value Value::UInt(uint64_t i)
{
    Value v;
    v.kind = Kind::UInt;
    v.unsigned_integer = i;
    return v;
}

Value Value::Str(std::string s)
{
    Value v;
    v.kind = Kind::String;
    v.string = std::move(s);
    return v;
}

Value Value::OfData(std::vector<uint8_t> d)
{
    Value v;
    v.kind = Kind::Data;
    v.data = std::move(d);
    return v;
}

Value Value::Array()
{
    Value v;
    v.kind = Kind::Array;
    return v;
}

Value Value::Dict()
{
    Value v;
    v.kind = Kind::Dict;
    return v;
}

const Value* Value::find(std::string_view key) const
{
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == key) {
            return &values[i];
        }
    }
    return nullptr;
}

void Value::set(std::string key, Value v)
{
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == key) {
            values[i] = std::move(v);
            return;
        }
    }
    keys.push_back(std::move(key));
    values.push_back(std::move(v));
}

void Value::push(Value v)
{
    array.push_back(std::move(v));
}

std::string Value::as_string_or(std::string_view fallback) const
{
    return kind == Kind::String ? string : std::string(fallback);
}

int64_t Value::as_int_or(int64_t fallback) const
{
    if (kind == Kind::Int) {
        return integer;
    }
    if (kind == Kind::Real) {
        if (std::isfinite(real) && real >= -0x1p63 && real < 0x1p63) {
            return static_cast<int64_t>(real);
        }
        return fallback;
    }
    return fallback;
}

bool Value::as_bool_or(bool fallback) const
{
    return kind == Kind::Bool ? boolean : fallback;
}

uint64_t Value::as_uint_or(uint64_t fallback) const
{
    if (kind == Kind::UInt) {
        return unsigned_integer;
    }
    if (kind == Kind::Int && integer >= 0) {
        return static_cast<uint64_t>(integer);
    }
    return fallback;
}

} // namespace maa::ios::plist
