#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::plist
{

inline constexpr std::size_t kMaxInput = 8u << 20;

enum class Kind
{
    Bool,
    Int,
    Real,
    String,
    Data,
    Array,
    Dict,
    UInt
};

/// XML plist 的协议值类型，编解码使用 pugixml。
///
/// dict 用 keys/values 平行数组而不是 vector<pair<string, Value>>：后者要在
/// Value 尚不完整时实例化 pair，标准库不保证支持。平行数组同时保留插入顺序。
struct Value
{
    Kind kind = Kind::Bool;
    bool boolean = false;
    int64_t integer = 0;
    uint64_t unsigned_integer = 0;
    double real = 0;
    std::string string;
    std::vector<uint8_t> data;
    std::vector<Value> array;
    std::vector<std::string> keys;
    std::vector<Value> values;

    static Value Bool(bool b);
    static Value Int(int64_t i);
    static Value UInt(uint64_t i);
    static Value Str(std::string s);
    static Value OfData(std::vector<uint8_t> d);
    static Value Array();
    static Value Dict();

    [[nodiscard]] bool is_dict() const { return kind == Kind::Dict; }

    [[nodiscard]] bool is_array() const { return kind == Kind::Array; }

    [[nodiscard]] bool is_string() const { return kind == Kind::String; }

    [[nodiscard]] bool is_int() const { return kind == Kind::Int; }

    [[nodiscard]] bool is_uint() const { return kind == Kind::UInt; }

    [[nodiscard]] bool is_bool() const { return kind == Kind::Bool; }

    /// 返回 nullptr 表示键不存在。
    [[nodiscard]] const Value* find(std::string_view key) const;
    void set(std::string key, Value v);
    void push(Value v);

    [[nodiscard]] std::string as_string_or(std::string_view fallback = { }) const;
    [[nodiscard]] int64_t as_int_or(int64_t fallback = 0) const;
    [[nodiscard]] uint64_t as_uint_or(uint64_t fallback = 0) const;
    [[nodiscard]] bool as_bool_or(bool fallback = false) const;
};

/// 解析 XML plist。失败返回 nullopt 并把原因写进 err。
///
/// 有嵌套深度与输入长度上限：解析的是外部输入，不能让它把栈打爆。
std::optional<Value> parse(std::string_view xml, std::string* err = nullptr);

/// 序列化成 XML plist（含标准 prolog）。
[[nodiscard]] std::string write(const Value& v);

} // namespace maa::ios::plist
