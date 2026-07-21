# meojson API Reference

## json::value

### Constructors

```cpp
value();                          // null
value(bool b);
value(int/unsigned/long/long long/float/double/long double num);
value(const char* str);
value(std::string str);
value(std::string_view str);
value(std::nullptr_t);            // null
value(const array& arr);
value(const object& obj);
value(std::initializer_list<object::value_type>);  // → object

// From enum (with MEOJSON_ENUM_RANGE)
value(enum_t e);                  // → string (default) or number (MEOJSON_ENUM_AS_NUMBER)

// From struct with to_json() member
value(const T& val);              // calls val.to_json()

// From ext::jsonization<T> specialization
value(const T& val);              // calls ext::jsonization<T>().to_json(val)

// From nullable wrappers
value(const std::optional<T>&);   // null if empty
value(const std::shared_ptr<T>&); // null if nullptr
value(const std::unique_ptr<T>&); // null if nullptr

// From collections
value(const std::vector<T>&);     // → array
value(const std::set<T>&);        // → array
value(const std::array<T,N>&);    // → array
value(const std::pair<A,B>&);     // → array [a, b]
value(const std::tuple<Ts...>&);  // → array [...]
value(const std::variant<Ts...>&);

// From maps
value(const std::map<std::string, T>&);           // → object
value(const std::unordered_map<std::string, T>&);  // → object

// Deleted
value(char) = delete;
value(wchar_t) = delete;
```

### Type Query

```cpp
bool valid() const noexcept;       // not invalid type
bool empty() const noexcept;       // null, empty string/array/object
bool is_null() const noexcept;
bool is_boolean() const noexcept;
bool is_number() const noexcept;
bool is_string() const noexcept;
bool is_array() const noexcept;
bool is_object() const noexcept;

template <typename T>
bool is() const noexcept;          // check if convertible to T

template <typename T>
bool all() const;                  // all elements are convertible to T (array)

value_type type() const noexcept;  // enum value_type
std::string type_name() const;
```

### Access

```cpp
bool as_boolean() const;
int as_integer() const;
double as_double() const;
std::string as_string() const;
std::string_view as_string_view() const;
const array& as_array() const;
const object& as_object() const;
array& as_array();                 // mutable
object& as_object();               // mutable

template <typename T>
T as() const&;                     // explicit conversion

// Subscript
const value& operator[](size_t pos) const;
value& operator[](size_t pos);
value& operator[](const std::string& key);

const value& at(size_t pos) const;
const value& at(const std::string& key) const;
```

### Safe Lookup

```cpp
bool contains(const std::string& key) const;
bool contains(size_t pos) const;
bool exists(const std::string& key) const;
bool exists(size_t pos) const;

// find — returns std::optional
template <typename T = value>
std::optional<T> find(size_t pos) const;
template <typename T = value>
std::optional<T> find(const std::string& key) const;

// get — chained key lookup with default
// Usage: get(key1, key2, ..., default_value)
template <typename... key_then_default_value_t>
auto get(key_then_default_value_t&&... keys_then_default_value) const;
```

### Modification

```cpp
bool erase(size_t pos);
bool erase(const std::string& key);
void clear() noexcept;

template <typename... args_t>
decltype(auto) emplace(args_t&&... args);

// Object merge
value operator|(const object& rhs) const&;
value& operator|=(const object& rhs);

// Array concat
value operator+(const array& rhs) const&;
value& operator+=(const array& rhs);
```

### Output

```cpp
std::string dumps(std::optional<size_t> indent = std::nullopt) const;
std::string format(size_t indent = 4) const;
std::string to_string() const;    // raw (no re-formatting)
```

## json::array

Wraps `std::vector<json::value>`. Supports STL container interface (begin/end/size/empty/emplace_back/push_back/operator[]).

Additional:
```cpp
std::string dumps(std::optional<size_t> indent = std::nullopt) const;
std::string format(size_t indent = 4) const;

template <typename T>
T as() const&;    // convert to std::vector<T>, std::array<T,N>, std::tuple<...>

template <typename T = value>
std::optional<T> find(size_t pos) const;
```

## json::object

Wraps `std::map<std::string, json::value>`. Supports STL map interface (begin/end/size/empty/contains/emplace/insert/erase/operator[]).

Additional:
```cpp
std::string dumps(std::optional<size_t> indent = std::nullopt) const;
std::string format(size_t indent = 4) const;

bool exists(const std::string& key) const;

template <typename T = value>
std::optional<T> find(const std::string& key) const;

// Chained key lookup with default (same as value::get)
template <typename... key_then_default_value_t>
auto get(key_then_default_value_t&&... keys_then_default_value) const;

// Merge
object operator|(const object& rhs) const&;
object& operator|=(const object& rhs);
```

## Parsing Functions

```cpp
namespace json {
// Returns std::optional<value>

template <typename T>
std::optional<value> parse(const T& content);        // from string
std::optional<value> parse(const char* content);

template <typename T>
std::optional<value> parsec(const T& content);       // JSONC (with comments)
std::optional<value> parsec(const char* content);

std::optional<value> parse(std::istream& is, bool check_bom = false, bool with_comments = false);

template <typename path_t>
std::optional<value> open(const path_t& path, bool check_bom = false, bool with_comments = false);
}
```

## User-Defined Literals

```cpp
using namespace json::literals;

auto v = R"({"key": "value"})"_json;      // → json::value
auto v = R"({"key": "value"})"_jvalue;    // → json::value
auto a = R"([1, 2, 3])"_jarray;           // → json::array
auto o = R"({"key": "value"})"_jobject;   // → json::object
```

## ext::jsonization Template

```cpp
namespace json::ext {
template <typename T>
class jsonization {
public:
    // Required for serialization
    json::value to_json(const T& val) const;

    // Required for deserialization
    bool check_json(const json::value& json) const;
    bool from_json(const json::value& json, T& val) const;

    // Optional: move semantics
    json::value move_to_json(T&& val) const;
    bool move_from_json(json::value&& json, T& val) const;
};
}
```

## MaaFramework ext::jsonization Specializations

Defined in `source/MaaUtils/include/MaaUtils/JsonExt.hpp`:

| Type | to_json | from_json | Notes |
|------|---------|-----------|-------|
| `std::wstring` | UTF-8 string | from UTF-8 | via `from_u16` / `to_u16` |
| `std::filesystem::path` | UTF-8 string | from UTF-8 | via `path_to_utf8_string` / `path` |
| `std::chrono::milliseconds` | `"123ms"` | N/A | to_json only |
| `cv::Point` | `[x, y]` | from `[x, y]` | |
| `cv::Rect` | `[x, y, w, h]` | from `[x, y, w, h]` | |
| `cv::Size` | `[w, h]` | from `[w, h]` | |
| `cv::Mat` | `[rows, cols, type]` | N/A | to_json only |
| `T` with `operator<<` | `oss.str()` | N/A | Fallback, to_json only. Only if T is not constructible to value/array/object |

## Macro Reference

| Macro | Effect |
|-------|--------|
| `MEO_JSONIZATION(fields...)` | Generate `to_json()`, `check_json()`, `from_json()` |
| `MEO_TOJSON(fields...)` | Generate `to_json()` only |
| `MEO_FROMJSON(fields...)` | Generate `from_json()` only |
| `MEO_CHECKJSON(fields...)` | Generate `check_json()` only |
| `MEO_OPT` | Next field is optional in from_json (skip if missing) |
| `MEO_KEY("key")` | Override JSON key name for next field |

### Macro Generated Signatures

```cpp
// MEO_TOJSON generates:
json::value to_json() const;

// MEO_CHECKJSON generates:
bool check_json(const json::value& in) const;
bool check_json(const json::value& in, std::string& error_key) const;

// MEO_FROMJSON generates:
bool from_json(const json::value& in);
bool from_json(const json::value& in, std::string& error_key);
```
