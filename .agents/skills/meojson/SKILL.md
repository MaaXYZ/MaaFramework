---
name: meojson
description: Guide for using the meojson C++ JSON library in MaaFramework. Use when writing code that involves JSON parsing, serialization, struct jsonization (MEO_JSONIZATION), json::value manipulation, ext::jsonization custom type support, or logging with json values.
---

# meojson in MaaFramework

meojson is a header-only, zero-dependency C++ JSON library. In MaaFramework it lives at `source/MaaUtils/include/meojson/` and is included via `<meojson/json.hpp>`.

## Core Types

| Type | Description |
|------|-------------|
| `json::value` | Universal JSON value (null/bool/number/string/array/object) |
| `json::array` | JSON array, wraps `std::vector<json::value>` |
| `json::object` | JSON object, wraps `std::map<std::string, json::value>` |

## Parsing

```cpp
#include <meojson/json.hpp>

// From string — returns std::optional<json::value>
auto opt = json::parse(str);
if (!opt) { /* parse failed */ }

// From file
auto opt = json::open("/path/to/file.json");

// JSONC (with comments)
auto opt = json::parsec(str);

// From istream
auto opt = json::parse(istream, /*check_bom=*/false, /*with_comments=*/false);
```

**MaaFramework pattern** — always check parse result and type:

```cpp
auto ov_opt = json::parse(pipeline_override);
if (!ov_opt) {
    LogError << "failed to parse" << VAR(pipeline_override);
    return MaaInvalidId;
}
if (!ov_opt->is_object()) {
    LogError << "json is not object" << VAR(pipeline_override);
    return MaaInvalidId;
}
auto& obj = ov_opt->as_object();
```

## Constructing Values

```cpp
json::value v1 = 42;
json::value v2 = "hello";
json::value v3 = true;
json::value v4 = nullptr;    // null

json::array arr { 1, 2, "three" };
json::object obj {
    { "key1", "value1" },
    { "key2", 42 },
};

// From STL containers (implicit conversion)
std::vector<int> vec = {1, 2, 3};
json::value v5 = vec;                              // → JSON array

std::map<std::string, int> m = {{"a", 1}};
json::value v6 = m;                                // → JSON object

// Initializer list for object
json::value v7 = { {"name", "test"}, {"count", 5} };
```

## Reading Values

### Type Checking

```cpp
v.valid()       // not invalid
v.is_null()
v.is_boolean()
v.is_number()
v.is_string()
v.is_array()
v.is_object()
v.is<int>()     // check if convertible to type
```

### Direct Access (throws on type mismatch)

```cpp
v.as_string()       // → std::string
v.as_string_view()  // → std::string_view (no copy)
v.as_integer()
v.as_double()
v.as_boolean()
v.as_array()        // → const json::array&
v.as_object()       // → const json::object&
v.as<T>()           // → T (explicit conversion)
```

### Safe Access

```cpp
// find() returns std::optional<T>, safe for missing keys
auto opt = v.find<std::string>("key");
if (opt) {
    std::string s = *opt;
}

// get() with default value — supports chained keys
std::string s = v.get("key", "default_value");
int n = v.get("a", "b", 0);  // v["a"]["b"], default 0

// exists() / contains() — check key presence
if (v.exists("key")) { ... }
```

**MaaFramework pattern** — `find` + type check:

```cpp
template <typename OutT>
bool get_and_check_value(const json::value& input, const std::string& key, OutT& output, const OutT& default_val)
{
    auto opt = input.find<OutT>(key);
    if (!opt) {
        if (input.exists(key)) {
            LogError << "type error" << VAR(key) << VAR(input);
            return false;
        }
        output = default_val;
    }
    else {
        output = *opt;
    }
    return true;
}
```

### Subscript Access

```cpp
const json::value& v2 = v["key"];      // object access
const json::value& v3 = v[0];          // array access
v["key"] = "new_value";                // mutable access (creates key if missing)
```

### Iteration

```cpp
// Array
for (const auto& item : v.as_array()) { ... }

// Object
for (const auto& [key, val] : v.as_object()) { ... }
```

## Serialization

```cpp
v.dumps()               // compact string
v.dumps(4)              // pretty print with indent=4
v.format()              // same as dumps(4)
v.to_string()           // raw string (no re-formatting)
```

## Object Merge Operator

```cpp
// Merge objects with | operator (right side wins on conflict)
json::value merged = obj1 | obj2;
obj1 |= obj2;
```

## MEO_JSONIZATION — Struct ↔ JSON

The macro `MEO_JSONIZATION(fields...)` generates `to_json()`, `check_json()`, `from_json()` member functions for a struct.

### Basic Usage

```cpp
struct MyData {
    std::string name;
    int count = 0;
    std::vector<int> items;

    MEO_JSONIZATION(name, count, items);
};

// Serialize
MyData data { "test", 42, {1, 2, 3} };
json::value j = data;               // implicit via to_json()
// → {"name":"test","count":42,"items":[1,2,3]}

// Deserialize
json::value j2 = json::parse(str).value();
MyData data2 = j2.as<MyData>();      // or static_cast<MyData>(j2)
```

### MEO_OPT — Optional Fields

By default all fields are **required** in `from_json()`. Prefix with `MEO_OPT` to make a field optional (keeps default value if missing):

```cpp
struct Config {
    std::string name;           // required
    int timeout = 5000;         // optional, default 5000
    std::string label;          // optional

    MEO_JSONIZATION(name, MEO_OPT timeout, MEO_OPT label);
};
```

### MEO_KEY — Override JSON Key Name

When the C++ field name differs from the JSON key (e.g. reserved words), use `MEO_KEY("json_key")`:

```cpp
struct JTemplateMatch {
    std::vector<std::string> template_;   // "template" is C++ keyword

    MEO_TOJSON(MEO_KEY("template") template_);
};
// JSON key will be "template", not "template_"
```

Combine with `MEO_OPT`:

```cpp
MEO_JSONIZATION(MEO_OPT MEO_KEY("default") default_);
```

### MEO_TOJSON / MEO_FROMJSON / MEO_CHECKJSON

These are sub-macros if you only need one direction:

| Macro | Generates |
|-------|-----------|
| `MEO_TOJSON(...)` | `to_json()` only |
| `MEO_FROMJSON(...)` | `from_json()` only |
| `MEO_CHECKJSON(...)` | `check_json()` only |
| `MEO_JSONIZATION(...)` | All three |

In MaaFramework, `PipelineTypesV2.h` (dump-only structs) uses `MEO_TOJSON` exclusively.

### Supported Field Types

`MEO_JSONIZATION` supports any type that `json::value` can convert to/from:

- Primitives: `int`, `double`, `bool`, `std::string`
- STL containers: `std::vector<T>`, `std::map<std::string, T>`, `std::array<T,N>`
- Nullable: `std::optional<T>`, `std::shared_ptr<T>`
- Tuple-like: `std::pair<A,B>`, `std::tuple<...>`
- Variant: `std::variant<Ts...>`
- Nested structs with `MEO_JSONIZATION` / `to_json()`
- Types with `ext::jsonization<T>` specialization
- `json::value`, `json::object`, `json::array` directly

## ext::jsonization — Custom Type Support

For types you don't own (or need special conversion logic), specialize `json::ext::jsonization<T>`:

```cpp
namespace json::ext {
template <>
class jsonization<cv::Rect> {
public:
    json::value to_json(const cv::Rect& rect) const {
        return json::array { rect.x, rect.y, rect.width, rect.height };
    }

    bool check_json(const json::value& json) const {
        return json.is<std::vector<int>>() && json.as_array().size() == 4;
    }

    bool from_json(const json::value& json, cv::Rect& rect) const {
        auto arr = json.as<std::vector<int>>();
        rect = cv::Rect(arr[0], arr[1], arr[2], arr[3]);
        return true;
    }
};
}
```

MaaFramework defines these in `MaaUtils/JsonExt.hpp`:
- `std::wstring` ↔ UTF-8 string
- `std::filesystem::path` ↔ UTF-8 string
- `std::chrono::milliseconds` → `"123ms"` string (to_json only)
- `cv::Point` ↔ `[x, y]`
- `cv::Rect` ↔ `[x, y, w, h]`
- `cv::Size` ↔ `[w, h]`
- `cv::Mat` → `[rows, cols, type]` (to_json only)
- Fallback for any type with `operator<<` → string

## Logging Integration

MaaFramework's `LogStream` converts **any value to JSON** for logging via `json::value` constructor:

```cpp
// In LoggerUtils.h — LogStream::stream():
json::value j(std::forward<T>(value));
buffer_ << (j.is_string() ? j.as_string() : j.dumps()) << sep.str;
```

This means you can log **any type** that `json::value` can construct from (including structs with `MEO_JSONIZATION` or `ext::jsonization`):

```cpp
cv::Rect rect(10, 20, 100, 200);
LogInfo << "rect:" << rect;
// Output: rect: [10, 20, 100, 200]

MyData data { "test", 42 };
LogDebug << VAR(data);
// Output: data: {"name":"test","count":42}
```

Strings are logged without quotes; other types are logged as `dumps()` output.

## Enum Reflection

meojson supports automatic enum ↔ string conversion. Use `MEOJSON_ENUM_RANGE` in the enum:

```cpp
enum class MyEnum {
    A,
    B,
    C,
    MEOJSON_ENUM_RANGE(A, C)
};

json::value j = MyEnum::B;    // → "B"
MyEnum e = j.as<MyEnum>();    // → MyEnum::B
```

## Common Pitfalls

1. **`json::parse` returns `std::optional`** — always check before use
2. **`as_*()` throws on type mismatch** — use `find()` or check `is_*()` first
3. **`char` is deleted** — `json::value('a')` won't compile; use `std::string` or `int`
4. **`ext::jsonization` lives in `json::ext` namespace** — don't forget the namespace
5. **`MEO_OPT` applies to the next field only** — each optional field needs its own `MEO_OPT`
6. **`MEO_KEY` goes after `MEO_OPT`** — order is `MEO_OPT MEO_KEY("key") field`
7. **Object merge `|` creates new value** — use `|=` for in-place merge
8. **`std::variant` serialization** — converts to/from the first matching alternative

## Quick Reference

For detailed API signatures and template constraints, see [reference.md](reference.md).
