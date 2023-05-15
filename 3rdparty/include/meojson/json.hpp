#pragma once

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#define MEOJSON_INLINE inline

namespace json
{
template <typename string_t>
class basic_value;
template <typename string_t>
class basic_array;
template <typename string_t>
class basic_object;

using default_string_t = std::string;

using value = basic_value<default_string_t>;
using array = basic_array<default_string_t>;
using object = basic_object<default_string_t>;

using wvalue = basic_value<std::wstring>;
using warray = basic_array<std::wstring>;
using wobject = basic_object<std::wstring>;

using u16value = basic_value<std::u16string>;
using u16array = basic_array<std::u16string>;
using u16object = basic_object<std::u16string>;

using u32value = basic_value<std::u32string>;
using u32array = basic_array<std::u32string>;
using u32object = basic_object<std::u32string>;

// *************************
// *     basic_value declare     *
// *************************

template <typename string_t>
class basic_value
{
    using array_ptr = std::unique_ptr<basic_array<string_t>>;
    using object_ptr = std::unique_ptr<basic_object<string_t>>;

public:
    enum class value_type : char
    {
        invalid,
        null,
        boolean,
        string,
        number,
        j_array,
        j_object
    };

    using var_t = std::variant<string_t, array_ptr, object_ptr>;
    using char_t = typename string_t::value_type;

public:
    basic_value();
    basic_value(const basic_value<string_t>& rhs);
    basic_value(basic_value<string_t>&& rhs) noexcept;

    basic_value(bool b);

    basic_value(int num);
    basic_value(unsigned num);
    basic_value(long num);
    basic_value(unsigned long num);
    basic_value(long long num);
    basic_value(unsigned long long num);
    basic_value(float num);
    basic_value(double num);
    basic_value(long double num);

    basic_value(const char_t* str);
    basic_value(string_t str);

    basic_value(basic_array<string_t> arr);
    basic_value(basic_object<string_t> obj);

    // Constructed from raw data
    template <typename... args_t>
    basic_value(value_type type, args_t&&... args);

    // Prohibit conversion of other types to basic_value
    template <typename value_t>
    basic_value(value_t) = delete;

    ~basic_value();

    bool valid() const noexcept { return _type != value_type::invalid; }
    bool empty() const noexcept { return is_null(); }
    bool is_null() const noexcept { return _type == value_type::null; }
    bool is_number() const noexcept { return _type == value_type::number; }
    bool is_boolean() const noexcept { return _type == value_type::boolean; }
    bool is_string() const noexcept { return _type == value_type::string; }
    bool is_array() const noexcept { return _type == value_type::j_array; }
    bool is_object() const noexcept { return _type == value_type::j_object; }
    template <typename value_t>
    bool is() const noexcept;

    bool contains(const string_t& key) const;
    bool contains(size_t pos) const;
    bool exists(const string_t& key) const { return contains(key); }
    bool exists(size_t pos) const { return contains(pos); }
    value_type type() const noexcept { return _type; }
    const basic_value<string_t>& at(size_t pos) const;
    const basic_value<string_t>& at(const string_t& key) const;

    // usage: get(key, key_child, ..., default_value);
    template <typename... key_then_default_value_t>
    decltype(auto) get(key_then_default_value_t&&... keys_then_default_value) const;

    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(size_t pos) const;
    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(const string_t& key) const;

    bool as_boolean() const;
    int as_integer() const;
    // unsigned as_unsigned() const;
    long as_long() const;
    unsigned long as_unsigned_long() const;
    long long as_long_long() const;
    unsigned long long as_unsigned_long_long() const;
    float as_float() const;
    double as_double() const;
    long double as_long_double() const;
    const string_t as_string() const;
    const basic_array<string_t>& as_array() const;
    const basic_object<string_t>& as_object() const;
    template <typename value_t>
    value_t as() const;

    basic_array<string_t>& as_array();
    basic_object<string_t>& as_object();

    template <typename... args_t>
    decltype(auto) array_emplace(args_t&&... args);
    template <typename... args_t>
    decltype(auto) object_emplace(args_t&&... args);
    void clear() noexcept;

    // return raw string
    const string_t to_string() const;
    const string_t format(bool ordered = false, string_t shift_str = "    ", size_t basic_shift_count = 0) const;

    basic_value<string_t>& operator=(const basic_value<string_t>& rhs);
    basic_value<string_t>& operator=(basic_value<string_t>&&) noexcept;

    bool operator==(const basic_value<string_t>& rhs) const;
    bool operator!=(const basic_value<string_t>& rhs) const { return !(*this == rhs); }

    const basic_value<string_t>& operator[](size_t pos) const;
    basic_value<string_t>& operator[](size_t pos);
    basic_value<string_t>& operator[](const string_t& key);
    basic_value<string_t>& operator[](string_t&& key);

    basic_value<string_t> operator|(const basic_object<string_t>& rhs) &;
    basic_value<string_t> operator|(basic_object<string_t>&& rhs) &;
    basic_value<string_t> operator|(const basic_object<string_t>& rhs) &&;
    basic_value<string_t> operator|(basic_object<string_t>&& rhs) &&;

    basic_value<string_t>& operator|=(const basic_object<string_t>& rhs);
    basic_value<string_t>& operator|=(basic_object<string_t>&& rhs);

    basic_value<string_t> operator+(const basic_array<string_t>& rhs) &;
    basic_value<string_t> operator+(basic_array<string_t>&& rhs) &;
    basic_value<string_t> operator+(const basic_array<string_t>& rhs) &&;
    basic_value<string_t> operator+(basic_array<string_t>&& rhs) &&;

    basic_value<string_t>& operator+=(const basic_array<string_t>& rhs);
    basic_value<string_t>& operator+=(basic_array<string_t>&& rhs);

    explicit operator bool() const { return as_boolean(); }
    explicit operator int() const { return as_integer(); }
    explicit operator long() const { return as_long(); }
    explicit operator unsigned long() const { return as_unsigned_long(); }
    explicit operator long long() const { return as_long_long(); }
    explicit operator unsigned long long() const { return as_unsigned_long_long(); }
    explicit operator float() const { return as_float(); }
    explicit operator double() const { return as_double(); }
    explicit operator long double() const { return as_long_double(); }
    explicit operator string_t() const { return as_string(); }

private:
    static var_t deep_copy(const var_t& src);

    template <typename... key_then_default_value_t, size_t... keys_indexes_t>
    decltype(auto) get(std::tuple<key_then_default_value_t...> keys_then_default_value,
                       std::index_sequence<keys_indexes_t...>) const;

    template <typename value_t, typename first_key_t, typename... rest_keys_t>
    decltype(auto) get_helper(value_t&& default_value, first_key_t&& first, rest_keys_t&&... rest) const;
    template <typename value_t, typename unique_key_t>
    decltype(auto) get_helper(value_t&& default_value, unique_key_t&& first) const;

    const string_t& as_basic_type_str() const;
    string_t& as_basic_type_str();

    value_type _type = value_type::null;
    var_t _raw_data;
};

// *************************
// *     utils declare     *
// *************************

template <typename string_t = default_string_t>
const basic_value<string_t> invalid_value();

template <typename string_t>
std::ostream& operator<<(std::ostream& out, const basic_value<string_t>& val);

template <typename string_t>
static string_t true_string()
{
    return { 't', 'r', 'u', 'e' };
}

template <typename string_t>
static string_t false_string()
{
    return { 'f', 'a', 'l', 's', 'e' };
}

template <typename string_t>
static string_t null_string()
{
    return { 'n', 'u', 'l', 'l' };
}

// *************************
// *     basic_array declare     *
// *************************
template <typename string_t>
class basic_array
{
public:
    using raw_array = std::vector<basic_value<string_t>>;
    using value_type = typename raw_array::value_type;
    using iterator = typename raw_array::iterator;
    using const_iterator = typename raw_array::const_iterator;
    using reverse_iterator = typename raw_array::reverse_iterator;
    using const_reverse_iterator = typename raw_array::const_reverse_iterator;
    using char_t = typename string_t::value_type;

public:
    basic_array() = default;
    basic_array(const basic_array<string_t>& rhs) = default;
    basic_array(basic_array<string_t>&& rhs) noexcept = default;
    basic_array(const raw_array& arr);
    basic_array(raw_array&& arr) noexcept;
    basic_array(std::initializer_list<typename raw_array::value_type> init_list);
    basic_array(typename raw_array::size_type size);

    explicit basic_array(const basic_value<string_t>& val);
    explicit basic_array(basic_value<string_t>&& val);
    template <typename array_t, typename _ = std::enable_if_t<
                                    std::is_constructible_v<basic_value<string_t>, typename array_t::value_type>>>
    basic_array(array_t arr);

    ~basic_array() noexcept = default;

    bool empty() const noexcept { return _array_data.empty(); }
    size_t size() const noexcept { return _array_data.size(); }
    bool contains(size_t pos) const { return pos < _array_data.size(); }
    bool exists(size_t pos) const { return contains(pos); }
    const basic_value<string_t>& at(size_t pos) const;
    const string_t to_string() const;
    const string_t format(bool ordered = false, string_t shift_str = "    ", size_t basic_shift_count = 0) const;

    bool get(size_t pos, bool default_value) const;
    int get(size_t pos, int default_value) const;
    long get(size_t pos, long default_value) const;
    unsigned long get(size_t pos, unsigned default_value) const;
    long long get(size_t pos, long long default_value) const;
    unsigned long long get(size_t pos, unsigned long long default_value) const;
    float get(size_t pos, float default_value) const;
    double get(size_t pos, double default_value) const;
    long double get(size_t pos, long double default_value) const;
    const string_t get(size_t pos, string_t default_value) const;
    const string_t get(size_t pos, const char_t* default_value) const;
    const basic_value<string_t>& get(size_t pos) const;

    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(size_t pos) const;

    template <typename... args_t>
    decltype(auto) emplace_back(args_t&&... args);

    void clear() noexcept;
    // void erase(size_t pos);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    reverse_iterator rbegin() noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    const_reverse_iterator crend() const noexcept;

    const basic_value<string_t>& operator[](size_t pos) const;
    basic_value<string_t>& operator[](size_t pos);

    basic_array<string_t> operator+(const basic_array<string_t>& rhs) &;
    basic_array<string_t> operator+(basic_array<string_t>&& rhs) &;
    basic_array<string_t> operator+(const basic_array<string_t>& rhs) &&;
    basic_array<string_t> operator+(basic_array<string_t>&& rhs) &&;

    basic_array<string_t>& operator+=(const basic_array<string_t>& rhs);
    basic_array<string_t>& operator+=(basic_array<string_t>&& rhs);

    basic_array<string_t>& operator=(const basic_array<string_t>&) = default;
    basic_array<string_t>& operator=(basic_array<string_t>&&) noexcept = default;

    bool operator==(const basic_array<string_t>& rhs) const;
    bool operator!=(const basic_array<string_t>& rhs) const { return !(*this == rhs); }

    // const raw_array &raw_data() const;

private:
    raw_array _array_data;
};

template <typename string_t>
std::ostream& operator<<(std::ostream& out, const basic_array<string_t>& arr);

// *************************
// *     basic_object declare    *
// *************************
template <typename string_t>
class basic_object
{
public:
    using raw_object = std::unordered_map<string_t, basic_value<string_t>>;
    using value_type = typename raw_object::value_type;
    using iterator = typename raw_object::iterator;
    using const_iterator = typename raw_object::const_iterator;
    using char_t = typename string_t::value_type;

public:
    basic_object() = default;
    basic_object(const basic_object<string_t>& rhs) = default;
    basic_object(basic_object<string_t>&& rhs) noexcept = default;
    basic_object(const raw_object& raw_obj);
    basic_object(raw_object&& raw_obj);
    basic_object(std::initializer_list<value_type> init_list);
    explicit basic_object(const basic_value<string_t>& val);
    explicit basic_object(basic_value<string_t>&& val);
    template <typename map_t,
              typename _ = std::enable_if_t<std::is_constructible_v<value_type, typename map_t::value_type>>>
    basic_object(map_t map);

    ~basic_object() = default;

    bool empty() const noexcept { return _object_data.empty(); }
    size_t size() const noexcept { return _object_data.size(); }
    bool contains(const string_t& key) const;
    bool exists(const string_t& key) const { return contains(key); }
    const basic_value<string_t>& at(const string_t& key) const;
    const string_t to_string() const;
    const string_t format(bool ordered = false, string_t shift_str = "    ", size_t basic_shift_count = 0) const;

    bool get(const string_t& key, bool default_value) const;
    int get(const string_t& key, int default_value) const;
    long get(const string_t& key, long default_value) const;
    unsigned long get(const string_t& key, unsigned default_value) const;
    long long get(const string_t& key, long long default_value) const;
    unsigned long long get(const string_t& key, unsigned long long default_value) const;
    float get(const string_t& key, float default_value) const;
    double get(const string_t& key, double default_value) const;
    long double get(const string_t& key, long double default_value) const;
    const string_t get(const string_t& key, string_t default_value) const;
    const string_t get(const string_t& key, const char_t* default_value) const;
    const basic_value<string_t>& get(const string_t& key) const;

    template <typename value_t = basic_value<string_t>>
    std::optional<value_t> find(const string_t& key) const;

    template <typename... args_t>
    decltype(auto) emplace(args_t&&... args);
    template <typename... args_t>
    decltype(auto) insert(args_t&&... args);

    void clear() noexcept;
    bool erase(const string_t& key);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    basic_value<string_t>& operator[](const string_t& key);
    basic_value<string_t>& operator[](string_t&& key);

    basic_object<string_t> operator|(const basic_object<string_t>& rhs) &;
    basic_object<string_t> operator|(basic_object<string_t>&& rhs) &;
    basic_object<string_t> operator|(const basic_object<string_t>& rhs) &&;
    basic_object<string_t> operator|(basic_object<string_t>&& rhs) &&;

    basic_object<string_t>& operator|=(const basic_object<string_t>& rhs);
    basic_object<string_t>& operator|=(basic_object<string_t>&& rhs);

    basic_object<string_t>& operator=(const basic_object<string_t>&) = default;
    basic_object<string_t>& operator=(basic_object<string_t>&&) = default;

    bool operator==(const basic_object<string_t>& rhs) const;
    bool operator!=(const basic_object<string_t>& rhs) const { return !(*this == rhs); }

    // const raw_object &raw_data() const;

private:
    raw_object _object_data;
};

template <typename string_t>
std::ostream& operator<<(std::ostream& out, const basic_object<string_t>& obj);

// *************************
// *   exception declare   *
// *************************
class exception : public std::exception
{
public:
    exception() = default;
    exception(const std::string& msg) : _what(msg) {}

    exception(const exception&) = default;
    exception& operator=(const exception&) = default;
    exception(exception&&) = default;
    exception& operator=(exception&&) = default;

    virtual ~exception() noexcept override = default;

    virtual const char* what() const noexcept override { return _what.empty() ? "Unknown exception" : _what.c_str(); }

protected:
    std::string _what;
};

// *************************
// *      helper declare      *
// *************************
template <typename string_t>
string_t unescape_string(string_t str);
template <typename string_t>
string_t escape_string(string_t str);

// *************************
// *       basic_value<string_t> impl      *
// *************************
template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value() = default;

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(const basic_value<string_t>& rhs)
    : _type(rhs._type), _raw_data(deep_copy(rhs._raw_data))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(basic_value<string_t>&& rhs) noexcept = default;

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(bool b)
    : _type(value_type::boolean), _raw_data(string_t(b ? true_string<string_t>() : false_string<string_t>()))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(int num) : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(unsigned num)
    : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(long num) : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(unsigned long num)
    : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(long long num)
    : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(unsigned long long num)
    : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(float num) : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(double num)
    : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(long double num)
    : _type(value_type::number), _raw_data(std::to_string(num))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(const char_t* str)
    : _type(value_type::string), _raw_data(unescape_string(string_t(str)))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(string_t str)
    : _type(value_type::string), _raw_data(unescape_string(std::move(str)))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(basic_array<string_t> arr)
    : _type(value_type::j_array), _raw_data(std::make_unique<basic_array<string_t>>(std::move(arr)))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::basic_value(basic_object<string_t> obj)
    : _type(value_type::j_object), _raw_data(std::make_unique<basic_object<string_t>>(std::move(obj)))
{
    ;
}

// for Pimpl
template <typename string_t>
MEOJSON_INLINE basic_value<string_t>::~basic_value() = default;

template <typename string_t>
template <typename value_t>
MEOJSON_INLINE bool basic_value<string_t>::is() const noexcept
{
    if constexpr (std::is_same_v<value_t, basic_value<string_t>>) {
        return true;
    }
    else if constexpr (std::is_same_v<value_t, bool>) {
        return _type == value_type::boolean;
    }
    else if constexpr (std::is_same_v<value_t, int> || std::is_same_v<value_t, unsigned> ||
                       std::is_same_v<value_t, long> || std::is_same_v<value_t, unsigned long> ||
                       std::is_same_v<value_t, long long> || std::is_same_v<value_t, unsigned long long> ||
                       std::is_same_v<value_t, float> || std::is_same_v<value_t, double> ||
                       std::is_same_v<value_t, long double>) {
        return _type == value_type::number;
    }
    else if constexpr (std::is_same_v<value_t, string_t>) {
        return _type == value_type::string;
    }
    else if constexpr (std::is_same_v<value_t, basic_array<string_t>>) {
        return _type == value_type::j_array;
    }
    else if constexpr (std::is_same_v<value_t, basic_object<string_t>>) {
        return _type == value_type::j_object;
    }
    else {
        static_assert(!sizeof(value_t), "Unsupported type");
    }
}

template <typename string_t>
MEOJSON_INLINE bool basic_value<string_t>::contains(const string_t& key) const
{
    return is_object() && as_object().contains(key);
}

template <typename string_t>
MEOJSON_INLINE bool basic_value<string_t>::contains(size_t pos) const
{
    return is_array() && as_array().contains(pos);
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_value<string_t>::at(size_t pos) const
{
    return as_array().at(pos);
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_value<string_t>::at(const string_t& key) const
{
    return as_object().at(key);
}

template <typename string_t>
template <typename... key_then_default_value_t>
MEOJSON_INLINE decltype(auto) basic_value<string_t>::get(key_then_default_value_t&&... keys_then_default_value) const
{
    return get(std::forward_as_tuple(keys_then_default_value...),
               std::make_index_sequence<sizeof...(keys_then_default_value) - 1> {});
}

template <typename string_t>
template <typename... key_then_default_value_t, size_t... keys_indexes_t>
MEOJSON_INLINE decltype(auto) basic_value<string_t>::get(
    std::tuple<key_then_default_value_t...> keys_then_default_value, std::index_sequence<keys_indexes_t...>) const
{
    constexpr unsigned long default_value_index = sizeof...(key_then_default_value_t) - 1;
    return get_helper(std::get<default_value_index>(keys_then_default_value),
                      std::get<keys_indexes_t>(keys_then_default_value)...);
}

template <typename string_t>
template <typename value_t, typename first_key_t, typename... rest_keys_t>
MEOJSON_INLINE decltype(auto) basic_value<string_t>::get_helper(value_t&& default_value, first_key_t&& first,
                                                                rest_keys_t&&... rest) const
{
    if constexpr (std::is_constructible_v<string_t, first_key_t>) {
        return is_object() ? as_object()
                                 .get(std::forward<first_key_t>(first))
                                 .get_helper(std::forward<value_t>(default_value), std::forward<rest_keys_t>(rest)...)
                           : default_value;
    }
    else if constexpr (std::is_integral_v<typename std::remove_reference<first_key_t>::type>) {
        return is_array() ? as_array()
                                .get(std::forward<first_key_t>(first))
                                .get_helper(std::forward<value_t>(default_value), std::forward<rest_keys_t>(rest)...)
                          : default_value;
    }
    else {
        static_assert(!sizeof(first_key_t), "Parameter must be integral or string_t constructible");
    }
}

template <typename string_t>
template <typename value_t, typename unique_key_t>
MEOJSON_INLINE decltype(auto) basic_value<string_t>::get_helper(value_t&& default_value, unique_key_t&& first) const
{
    if constexpr (std::is_constructible_v<string_t, unique_key_t>) {
        return is_object() ? as_object().get(std::forward<unique_key_t>(first), std::forward<value_t>(default_value))
                           : default_value;
    }
    else if constexpr (std::is_integral_v<std::decay_t<unique_key_t>>) {
        return is_array() ? as_array().get(std::forward<unique_key_t>(first), std::forward<value_t>(default_value))
                          : default_value;
    }
    else {
        static_assert(!sizeof(unique_key_t), "Parameter must be integral or string_t constructible");
    }
}

template <typename string_t>
template <typename value_t>
MEOJSON_INLINE std::optional<value_t> basic_value<string_t>::find(size_t pos) const
{
    return is_array() ? as_array().template find<value_t>(pos) : std::nullopt;
}

template <typename string_t>
template <typename value_t>
MEOJSON_INLINE std::optional<value_t> basic_value<string_t>::find(const string_t& key) const
{
    return is_object() ? as_object().template find<value_t>(key) : std::nullopt;
}

template <typename string_t>
MEOJSON_INLINE bool basic_value<string_t>::as_boolean() const
{
    if (is_boolean()) {
        if (const string_t& b_str = as_basic_type_str(); b_str == true_string<string_t>()) {
            return true;
        }
        else if (b_str == "false") {
            return false;
        }
        else {
            throw exception("Unknown Parse Error");
        }
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE int basic_value<string_t>::as_integer() const
{
    if (is_number()) {
        return std::stoi(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE long basic_value<string_t>::as_long() const
{
    if (is_number()) {
        return std::stol(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE unsigned long basic_value<string_t>::as_unsigned_long() const
{
    if (is_number()) {
        return std::stoul(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE long long basic_value<string_t>::as_long_long() const
{
    if (is_number()) {
        return std::stoll(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE unsigned long long basic_value<string_t>::as_unsigned_long_long() const
{
    if (is_number()) {
        return std::stoull(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE float basic_value<string_t>::as_float() const
{
    if (is_number()) {
        return std::stof(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE double basic_value<string_t>::as_double() const
{
    if (is_number()) {
        return std::stod(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE long double basic_value<string_t>::as_long_double() const
{
    if (is_number()) {
        return std::stold(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_value<string_t>::as_string() const
{
    if (is_string()) {
        return escape_string(as_basic_type_str());
    }
    else {
        throw exception("Wrong Type");
    }
}

template <typename string_t>
MEOJSON_INLINE const basic_array<string_t>& basic_value<string_t>::as_array() const
{
    if (is_array()) {
        return *std::get<array_ptr>(_raw_data);
    }

    throw exception("Wrong Type");
}

template <typename string_t>
MEOJSON_INLINE const basic_object<string_t>& basic_value<string_t>::as_object() const
{
    if (is_object()) {
        return *std::get<object_ptr>(_raw_data);
    }

    throw exception("Wrong Type or data empty");
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>& basic_value<string_t>::as_array()
{
    if (empty()) {
        *this = basic_array<string_t>();
    }

    if (is_array()) {
        return *std::get<array_ptr>(_raw_data);
    }

    throw exception("Wrong Type");
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>& basic_value<string_t>::as_object()
{
    if (empty()) {
        *this = basic_object<string_t>();
    }

    if (is_object()) {
        return *std::get<object_ptr>(_raw_data);
    }

    throw exception("Wrong Type or data empty");
}

template <typename string_t>
template <typename value_t>
MEOJSON_INLINE value_t basic_value<string_t>::as() const
{
    return static_cast<value_t>(*this);
}

template <typename string_t>
MEOJSON_INLINE const string_t& basic_value<string_t>::as_basic_type_str() const
{
    return std::get<string_t>(_raw_data);
}

template <typename string_t>
MEOJSON_INLINE string_t& basic_value<string_t>::as_basic_type_str()
{
    return std::get<string_t>(_raw_data);
}

template <typename string_t>
template <typename... args_t>
MEOJSON_INLINE decltype(auto) basic_value<string_t>::array_emplace(args_t&&... args)
{
    return as_array().emplace_back(std::forward<args_t>(args)...);
}

template <typename string_t>
template <typename... args_t>
MEOJSON_INLINE decltype(auto) basic_value<string_t>::object_emplace(args_t&&... args)
{
    return as_object().emplace(std::forward<args_t>(args)...);
}

template <typename string_t>
MEOJSON_INLINE void basic_value<string_t>::clear() noexcept
{
    *this = basic_value<string_t>();
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_value<string_t>::to_string() const
{
    switch (_type) {
    case value_type::null:
        return null_string<string_t>();
    case value_type::boolean:
    case value_type::number:
        return as_basic_type_str();
    case value_type::string:
        return '"' + as_basic_type_str() + '"';
    case value_type::j_array:
        return as_array().to_string();
    case value_type::j_object:
        return as_object().to_string();
    default:
        throw exception("Unknown basic_value Type");
    }
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_value<string_t>::format(bool ordered, string_t shift_str,
                                                            size_t basic_shift_count) const
{
    switch (_type) {
    case value_type::null:
        return null_string<string_t>();
    case value_type::boolean:
    case value_type::number:
        return as_basic_type_str();
    case value_type::string:
        return '"' + as_basic_type_str() + '"';
    case value_type::j_array:
        return as_array().format(ordered, shift_str, basic_shift_count);
    case value_type::j_object:
        return as_object().format(ordered, shift_str, basic_shift_count);
    default:
        throw exception("Unknown basic_value Type");
    }
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator=(const basic_value<string_t>& rhs)
{
    _type = rhs._type;
    _raw_data = deep_copy(rhs._raw_data);

    return *this;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator=(basic_value<string_t>&& rhs) noexcept = default;

template <typename string_t>
MEOJSON_INLINE bool basic_value<string_t>::operator==(const basic_value<string_t>& rhs) const
{
    if (_type != rhs._type) return false;

    switch (_type) {
    case value_type::null:
        return rhs.is_null();
    case value_type::boolean:
    case value_type::number:
    case value_type::string:
        return _raw_data == rhs._raw_data;
    case value_type::j_array:
        return as_array() == rhs.as_array();
    case value_type::j_object:
        return as_object() == rhs.as_object();
    default:
        throw exception("Unknown basic_value Type");
    }
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_value<string_t>::operator[](size_t pos) const
{
    // basic_array not support to create by operator[]

    return as_array()[pos];
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator[](size_t pos)
{
    // basic_array not support to create by operator[]

    return as_array()[pos];
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator[](const string_t& key)
{
    if (empty()) {
        *this = basic_object<string_t>();
    }

    return as_object()[key];
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator[](string_t&& key)
{
    if (empty()) {
        *this = basic_object<string_t>();
    }

    return as_object()[std::move(key)];
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator|(const basic_object<string_t>& rhs) &
{
    return as_object() | rhs;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator|(basic_object<string_t>&& rhs) &
{
    return as_object() | std::move(rhs);
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator|(const basic_object<string_t>& rhs) &&
{
    return std::move(as_object()) | rhs;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator|(basic_object<string_t>&& rhs) &&
{
    return std::move(as_object()) | std::move(rhs);
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator|=(const basic_object<string_t>& rhs)
{
    as_object() |= rhs;
    return *this;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator|=(basic_object<string_t>&& rhs)
{
    as_object() |= std::move(rhs);
    return *this;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator+(const basic_array<string_t>& rhs) &
{
    return as_array() + rhs;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator+(basic_array<string_t>&& rhs) &
{
    return as_array() + std::move(rhs);
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator+(const basic_array<string_t>& rhs) &&
{
    return std::move(as_array()) + rhs;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t> basic_value<string_t>::operator+(basic_array<string_t>&& rhs) &&
{
    return std::move(as_array()) + std::move(rhs);
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator+=(const basic_array<string_t>& rhs)
{
    as_array() += rhs;
    return *this;
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_value<string_t>::operator+=(basic_array<string_t>&& rhs)
{
    as_array() += std::move(rhs);
    return *this;
}

template <typename string_t>
template <typename... args_t>
basic_value<string_t>::basic_value(value_type type, args_t&&... args)
    : _type(type), _raw_data(std::forward<args_t>(args)...)
{
    static_assert(std::is_constructible_v<var_t, args_t...>, "Parameter can't be used to construct a var_t");
}

template <typename string_t>
MEOJSON_INLINE typename basic_value<string_t>::var_t basic_value<string_t>::deep_copy(const var_t& src)
{
    var_t dst;
    if (const auto string_ptr = std::get_if<string_t>(&src)) {
        dst = *string_ptr;
    }
    else if (const auto arr_ptr = std::get_if<array_ptr>(&src)) {
        dst = std::make_unique<basic_array<string_t>>(**arr_ptr);
    }
    else if (const auto obj_ptr = std::get_if<object_ptr>(&src)) {
        dst = std::make_unique<basic_object<string_t>>(**obj_ptr);
    }
    else {
        // maybe invalid_value
    }

    return dst;
}

// *************************
// *       basic_array impl      *
// *************************
template <typename string_t>
template <typename... args_t>
decltype(auto) basic_array<string_t>::emplace_back(args_t&&... args)
{
    static_assert(std::is_constructible_v<value_type, args_t...>,
                  "Parameter can't be used to construct a raw_array::value_type");
    return _array_data.emplace_back(std::forward<args_t>(args)...);
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>::basic_array(const raw_array& arr) : _array_data(arr)
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>::basic_array(raw_array&& arr) noexcept : _array_data(std::move(arr))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>::basic_array(std::initializer_list<value_type> init_list) : _array_data(init_list)
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>::basic_array(typename raw_array::size_type size) : _array_data(size)
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>::basic_array(const basic_value<string_t>& val)
    : basic_array<string_t>(val.as_array())
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>::basic_array(basic_value<string_t>&& val)
    : basic_array<string_t>(std::move(val.as_array()))
{
    ;
}

template <typename string_t>
template <typename array_t, typename _>
MEOJSON_INLINE basic_array<string_t>::basic_array(array_t arr)
{
    _array_data.assign(std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_array<string_t>::at(size_t pos) const
{
    return _array_data.at(pos);
}

template <typename string_t>
MEOJSON_INLINE void basic_array<string_t>::clear() noexcept
{
    _array_data.clear();
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_array<string_t>::to_string() const
{
    string_t str = "[";
    for (const basic_value<string_t>& val : _array_data) {
        str += val.to_string() + ",";
    }
    if (str.back() == ',') {
        str.pop_back();
    }
    str += "]";
    return str;
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_array<string_t>::format(bool ordered, string_t shift_str,
                                                            size_t basic_shift_count) const
{
    string_t shift;
    for (size_t i = 0; i != basic_shift_count + 1; ++i) {
        shift += shift_str;
    }

    string_t str = "[";
    for (const basic_value<string_t>& val : _array_data) {
        str += "\n" + shift + val.format(ordered, shift_str, basic_shift_count + 1) + ",";
    }
    if (str.back() == ',') {
        str.pop_back(); // pop last ','
    }

    str += '\n';
    for (size_t i = 0; i != basic_shift_count; ++i) {
        str += shift_str;
    }
    str += ']';
    return str;
}

template <typename string_t>
MEOJSON_INLINE bool basic_array<string_t>::get(size_t pos, bool default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_boolean()) {
            return basic_value.as_boolean();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE int basic_array<string_t>::get(size_t pos, int default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_integer();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE long basic_array<string_t>::get(size_t pos, long default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE unsigned long basic_array<string_t>::get(size_t pos, unsigned default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_unsigned_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE long long basic_array<string_t>::get(size_t pos, long long default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_long_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE unsigned long long basic_array<string_t>::get(size_t pos, unsigned long long default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_unsigned_long_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE float basic_array<string_t>::get(size_t pos, float default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_float();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE double basic_array<string_t>::get(size_t pos, double default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_double();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE long double basic_array<string_t>::get(size_t pos, long double default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_number()) {
            return basic_value.as_long_double();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_array<string_t>::get(size_t pos, string_t default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_string()) {
            return basic_value.as_string();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_array<string_t>::get(size_t pos, const char_t* default_value) const
{
    if (contains(pos)) {
        basic_value<string_t> basic_value = _array_data.at(pos);
        if (basic_value.is_string()) {
            return basic_value.as_string();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_array<string_t>::get(size_t pos) const
{
    if (contains(pos)) {
        return _array_data.at(pos);
    }
    else {
        static basic_value<string_t> null;
        return null;
    }
}

template <typename string_t>
template <typename value_t>
MEOJSON_INLINE std::optional<value_t> basic_array<string_t>::find(size_t pos) const
{
    static_assert(std::is_constructible_v<value_t, basic_value<string_t>>,
                  "Type can NOT be constructed by basic_value");
    if (!contains(pos)) {
        return std::nullopt;
    }
    const auto& val = _array_data.at(pos);
    return val.template is<value_t>() ? std::optional<value_t>(val.template as<value_t>()) : std::nullopt;
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::iterator basic_array<string_t>::begin() noexcept
{
    return _array_data.begin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::iterator basic_array<string_t>::end() noexcept
{
    return _array_data.end();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_iterator basic_array<string_t>::begin() const noexcept
{
    return _array_data.begin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_iterator basic_array<string_t>::end() const noexcept
{
    return _array_data.end();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_iterator basic_array<string_t>::cbegin() const noexcept
{
    return _array_data.cbegin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_iterator basic_array<string_t>::cend() const noexcept
{
    return _array_data.cend();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::reverse_iterator basic_array<string_t>::rbegin() noexcept
{
    return _array_data.rbegin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::reverse_iterator basic_array<string_t>::rend() noexcept
{
    return _array_data.rend();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::rbegin() const noexcept
{
    return _array_data.rbegin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::rend() const noexcept
{
    return _array_data.rend();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::crbegin() const noexcept
{
    return _array_data.crbegin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_array<string_t>::const_reverse_iterator basic_array<string_t>::crend() const noexcept
{
    return _array_data.crend();
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_array<string_t>::operator[](size_t pos)
{
    return _array_data[pos];
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_array<string_t>::operator[](size_t pos) const
{
    return _array_data[pos];
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t> basic_array<string_t>::operator+(const basic_array<string_t>& rhs) &
{
    basic_array<string_t> temp = *this;
    temp._array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
    return temp;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t> basic_array<string_t>::operator+(basic_array<string_t>&& rhs) &
{
    basic_array<string_t> temp = *this;
    temp._array_data.insert(_array_data.end(), std::make_move_iterator(rhs.begin()),
                            std::make_move_iterator(rhs.end()));
    return temp;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t> basic_array<string_t>::operator+(const basic_array<string_t>& rhs) &&
{
    _array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
    return std::move(*this);
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t> basic_array<string_t>::operator+(basic_array<string_t>&& rhs) &&
{
    _array_data.insert(_array_data.end(), std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return std::move(*this);
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>& basic_array<string_t>::operator+=(const basic_array<string_t>& rhs)
{
    _array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
    return *this;
}

template <typename string_t>
MEOJSON_INLINE basic_array<string_t>& basic_array<string_t>::operator+=(basic_array<string_t>&& rhs)
{
    _array_data.insert(_array_data.end(), std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return *this;
}

template <typename string_t>
MEOJSON_INLINE bool basic_array<string_t>::operator==(const basic_array<string_t>& rhs) const
{
    return _array_data == rhs._array_data;
}

// const raw_array &basic_array<string_t>::raw_data() const
// {
//     return _array_data;
// }

// *************************
// *      basic_object impl      *
// *************************
template <typename string_t>
template <typename... args_t>
decltype(auto) basic_object<string_t>::emplace(args_t&&... args)
{
    static_assert(std::is_constructible_v<value_type, args_t...>,
                  "Parameter can't be used to construct a raw_object::value_type");
    return _object_data.emplace(std::forward<args_t>(args)...);
}

template <typename string_t>
template <typename... args_t>
decltype(auto) basic_object<string_t>::insert(args_t&&... args)
{
    return _object_data.insert(std::forward<args_t>(args)...);
}

template <typename string_t>
MEOJSON_INLINE std::ostream& operator<<(std::ostream& out, const basic_array<string_t>& arr)
{
    // TODO: format output

    out << arr.to_string();
    return out;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>::basic_object(const raw_object& raw_obj) : _object_data(raw_obj)
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>::basic_object(raw_object&& raw_obj) : _object_data(std::move(raw_obj))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>::basic_object(std::initializer_list<typename raw_object::value_type> init_list)
{
    _object_data.reserve(init_list.size());
    for (const auto& [key, val] : init_list) {
        emplace(key, val);
    }
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>::basic_object(const basic_value<string_t>& val)
    : basic_object<string_t>(val.as_object())
{
    ;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>::basic_object(basic_value<string_t>&& val)
    : basic_object<string_t>(std::move(val.as_object()))
{
    ;
}

template <typename string_t>
MEOJSON_INLINE bool basic_object<string_t>::contains(const string_t& key) const
{
    return _object_data.find(key) != _object_data.cend();
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_object<string_t>::at(const string_t& key) const
{
    return _object_data.at(key);
}

template <typename string_t>
MEOJSON_INLINE void basic_object<string_t>::clear() noexcept
{
    _object_data.clear();
}

template <typename string_t>
MEOJSON_INLINE bool basic_object<string_t>::erase(const string_t& key)
{
    return _object_data.erase(key) > 0 ? true : false;
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_object<string_t>::to_string() const
{
    string_t str = "{";
    for (const auto& [key, val] : _object_data) {
        str += "\"" + unescape_string(key) + "\":" + val.to_string() + ",";
    }
    if (str.back() == ',') {
        str.pop_back();
    }
    str += "}";
    return str;
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_object<string_t>::format(bool ordered, string_t shift_str,
                                                             size_t basic_shift_count) const
{
    string_t shift;
    for (size_t i = 0; i != basic_shift_count + 1; ++i) {
        shift += shift_str;
    }

    string_t str = "{";
    auto append_kv = [&](const string_t& key, const basic_value<string_t>& val) {
        str += "\n" + shift + "\"" + unescape_string(key) +
               "\": " + val.format(ordered, shift_str, basic_shift_count + 1) + ",";
    };

    if (ordered) {
        std::vector<typename raw_object::const_iterator> ordered_data;
        for (auto it = _object_data.cbegin(); it != _object_data.cend(); ++it) {
            ordered_data.emplace_back(it);
        }
        std::sort(ordered_data.begin(), ordered_data.end(),
                  [](const auto& lhs, const auto& rhs) { return lhs->first < rhs->first; });
        for (const auto& it : ordered_data) {
            append_kv(it->first, it->second);
        }
    }
    else {
        for (const auto& [key, val] : _object_data) {
            append_kv(key, val);
        }
    }
    if (str.back() == ',') {
        str.pop_back(); // pop last ','
    }

    str += '\n';
    for (size_t i = 0; i != basic_shift_count; ++i) {
        str += shift_str;
    }
    str += '}';
    return str;
}

template <typename string_t>
MEOJSON_INLINE bool basic_object<string_t>::get(const string_t& key, bool default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_boolean()) {
            return basic_value.as_boolean();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE int basic_object<string_t>::get(const string_t& key, int default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_integer();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE long basic_object<string_t>::get(const string_t& key, long default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE unsigned long basic_object<string_t>::get(const string_t& key, unsigned default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_unsigned_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE long long basic_object<string_t>::get(const string_t& key, long long default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_long_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE unsigned long long basic_object<string_t>::get(const string_t& key,
                                                              unsigned long long default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_unsigned_long_long();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE float basic_object<string_t>::get(const string_t& key, float default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_float();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE double basic_object<string_t>::get(const string_t& key, double default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_double();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE long double basic_object<string_t>::get(const string_t& key, long double default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_number()) {
            return basic_value.as_long_double();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_object<string_t>::get(const string_t& key, string_t default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_string()) {
            return basic_value.as_string();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE const string_t basic_object<string_t>::get(const string_t& key, const char_t* default_value) const
{
    if (contains(key)) {
        basic_value<string_t> basic_value = _object_data.at(key);
        if (basic_value.is_string()) {
            return basic_value.as_string();
        }
        else {
            return default_value;
        }
    }
    else {
        return default_value;
    }
}

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t>& basic_object<string_t>::get(const string_t& key) const
{
    if (contains(key)) {
        return _object_data.at(key);
    }
    else {
        static basic_value<string_t> null;
        return null;
    }
}

template <typename string_t>
template <typename value_t>
MEOJSON_INLINE std::optional<value_t> basic_object<string_t>::find(const string_t& key) const
{
    static_assert(std::is_constructible_v<value_t, basic_value<string_t>>,
                  "value_t can NOT be constructed by basic_value");
    auto iter = _object_data.find(key);
    if (iter == _object_data.end()) {
        return std::nullopt;
    }
    const auto& val = iter->second;
    return val.template is<value_t>() ? std::optional<value_t>(val.template as<value_t>()) : std::nullopt;
}

template <typename string_t>
MEOJSON_INLINE typename basic_object<string_t>::iterator basic_object<string_t>::begin() noexcept
{
    return _object_data.begin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_object<string_t>::iterator basic_object<string_t>::end() noexcept
{
    return _object_data.end();
}

template <typename string_t>
MEOJSON_INLINE typename basic_object<string_t>::const_iterator basic_object<string_t>::begin() const noexcept
{
    return _object_data.begin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_object<string_t>::const_iterator basic_object<string_t>::end() const noexcept
{
    return _object_data.end();
}

template <typename string_t>
MEOJSON_INLINE typename basic_object<string_t>::const_iterator basic_object<string_t>::cbegin() const noexcept
{
    return _object_data.cbegin();
}

template <typename string_t>
MEOJSON_INLINE typename basic_object<string_t>::const_iterator basic_object<string_t>::cend() const noexcept
{
    return _object_data.cend();
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_object<string_t>::operator[](const string_t& key)
{
    return _object_data[key];
}

template <typename string_t>
MEOJSON_INLINE basic_value<string_t>& basic_object<string_t>::operator[](string_t&& key)
{
    return _object_data[std::move(key)];
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t> basic_object<string_t>::operator|(const basic_object<string_t>& rhs) &
{
    basic_object<string_t> temp = *this;
    temp._object_data.insert(rhs.begin(), rhs.end());
    return temp;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t> basic_object<string_t>::operator|(basic_object<string_t>&& rhs) &
{
    basic_object<string_t> temp = *this;
    // temp._object_data.merge(std::move(rhs._object_data));
    temp._object_data.insert(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return temp;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t> basic_object<string_t>::operator|(const basic_object<string_t>& rhs) &&
{
    _object_data.insert(rhs.begin(), rhs.end());
    return std::move(*this);
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t> basic_object<string_t>::operator|(basic_object<string_t>&& rhs) &&
{
    //_object_data.merge(std::move(rhs._object_data));
    _object_data.insert(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return std::move(*this);
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>& basic_object<string_t>::operator|=(const basic_object<string_t>& rhs)
{
    _object_data.insert(rhs.begin(), rhs.end());
    return *this;
}

template <typename string_t>
MEOJSON_INLINE basic_object<string_t>& basic_object<string_t>::operator|=(basic_object<string_t>&& rhs)
{
    _object_data.insert(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return *this;
}

template <typename string_t>
MEOJSON_INLINE bool basic_object<string_t>::operator==(const basic_object<string_t>& rhs) const
{
    return _object_data == rhs._object_data;
}

// const raw_object &basic_object<string_t>::raw_data() const
// {
//     return _object_data;
// }

template <typename string_t>
MEOJSON_INLINE std::ostream& operator<<(std::ostream& out, const basic_object<string_t>& obj)
{
    // TODO: format output

    out << obj.to_string();
    return out;
}

template <typename string_t>
template <typename map_t, typename _>
basic_object<string_t>::basic_object(map_t map)
{
    _object_data.insert(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()));
}

// *************************
// *     parser declare    *
// *************************
template <typename parsing_t, typename string_t = default_string_t>
class parser
{
public:
    using parsing_iter_t = typename parsing_t::const_iterator;

public:
    ~parser() noexcept = default;

    static std::optional<basic_value<string_t>> parse(const parsing_t& content);

private:
    parser(parsing_iter_t cbegin, parsing_iter_t cend) noexcept : _cur(cbegin), _end(cend) { ; }

    std::optional<basic_value<string_t>> parse();
    basic_value<string_t> parse_value();

    basic_value<string_t> parse_null();
    basic_value<string_t> parse_boolean();
    basic_value<string_t> parse_number();
    // parse and return a basic_value<string_t> whose type is value_type::string
    basic_value<string_t> parse_string();
    basic_value<string_t> parse_array();
    basic_value<string_t> parse_object();

    // parse and return a string_t
    std::optional<string_t> parse_stdstring();

    bool skip_whitespace() noexcept;
    bool skip_digit();

private:
    parsing_iter_t _cur;
    parsing_iter_t _end;
};

// *************************
// *      utils impl       *
// *************************

template <typename string_t>
MEOJSON_INLINE const basic_value<string_t> invalid_value()
{
    return basic_value<string_t>(basic_value<string_t>::value_type::invalid, typename basic_value<string_t>::var_t());
}

template <typename parsing_t, typename string_t = default_string_t>
MEOJSON_INLINE std::optional<basic_value<string_t>> parse(const parsing_t& content)
{
    return parser<parsing_t, string_t>::parse(content);
}

template <typename string_t>
MEOJSON_INLINE std::ostream& operator<<(std::ostream& out, const basic_value<string_t>& val)
{
    // TODO: format output

    out << val.to_string();
    return out;
}

// TODO
// std::istream &operator>>(std::istream &in, basic_value &val)
//{
//    return in;
//}

template <typename string_t>
MEOJSON_INLINE std::optional<basic_value<string_t>> open(std::ifstream& ifs, bool check_bom = false)
{
    if (!ifs.is_open()) {
        return std::nullopt;
    }

    ifs.seekg(0, std::ios::end);
    auto file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    string_t str(file_size, '\0');
    ifs.read(str.data(), file_size);

    if (check_bom) {
        using uchar = unsigned char;
        static constexpr uchar Bom_0 = 0xEF;
        static constexpr uchar Bom_1 = 0xBB;
        static constexpr uchar Bom_2 = 0xBF;

        if (str.size() >= 3 && static_cast<uchar>(str.at(0)) == Bom_0 && static_cast<uchar>(str.at(1)) == Bom_1 &&
            static_cast<uchar>(str.at(2)) == Bom_2) {
            str.assign(str.begin() + 3, str.end());
        }
    }

    return parse(str);
}

template <typename input_t, typename string_t = default_string_t>
MEOJSON_INLINE std::optional<basic_value<string_t>> open(const input_t& filepath, bool check_bom = false)
{
    using char_t = typename string_t::value_type;
    using ifstream_t = std::basic_ifstream<char_t, std::char_traits<char_t>>;

    static_assert(std::is_constructible_v<ifstream_t, input_t>, "input_t can't be used to construct a ifstream_t");

    ifstream_t ifs(filepath, std::ios::in);
    auto opt = open<string_t>(ifs, check_bom);
    ifs.close();
    return opt;
}

// *************************
// *      parser impl      *
// *************************

template <typename parsing_t, typename string_t>
MEOJSON_INLINE std::optional<basic_value<string_t>> parser<parsing_t, string_t>::parse(const parsing_t& content)
{
    return parser<parsing_t, string_t>(content.cbegin(), content.cend()).parse();
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE std::optional<basic_value<string_t>> parser<parsing_t, string_t>::parse()
{
    if (!skip_whitespace()) {
        return std::nullopt;
    }

    basic_value<string_t> result_value;
    switch (*_cur) {
    case '[':
        result_value = parse_array();
        break;
    case '{':
        result_value = parse_object();
        break;
    default: // A JSON payload should be an basic_object or basic_array
        return std::nullopt;
    }

    if (!result_value.valid()) {
        return std::nullopt;
    }

    // After the parsing is complete, there should be no more content other than
    // spaces behind
    if (skip_whitespace()) {
        return std::nullopt;
    }

    return result_value;
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_value()
{
    switch (*_cur) {
    case 'n':
        return parse_null();
    case 't':
    case 'f':
        return parse_boolean();
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_number();
    case '"':
        return parse_string();
    case '[':
        return parse_array();
    case '{':
        return parse_object();
    default:
        return invalid_value<string_t>();
    }
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_null()
{
    for (const auto& ch : null_string<string_t>()) {
        if (*_cur == ch) {
            ++_cur;
        }
        else {
            return invalid_value<string_t>();
        }
    }

    return basic_value<string_t>();
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_boolean()
{
    switch (*_cur) {
    case 't':
        for (const auto& ch : true_string<string_t>()) {
            if (*_cur == ch) {
                ++_cur;
            }
            else {
                return invalid_value<string_t>();
            }
        }
        return true;
    case 'f':
        for (const auto& ch : false_string<string_t>()) {
            if (*_cur == ch) {
                ++_cur;
            }
            else {
                return invalid_value<string_t>();
            }
        }
        return false;
    default:
        return invalid_value<string_t>();
    }
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_number()
{
    const auto first = _cur;
    if (*_cur == '-') {
        ++_cur;
    }

    // numbers cannot have leading zeroes
    if (_cur != _end && *_cur == '0' && _cur + 1 != _end && std::isdigit(*(_cur + 1))) {
        return invalid_value<string_t>();
    }

    if (!skip_digit()) {
        return invalid_value<string_t>();
    }

    if (*_cur == '.') {
        ++_cur;
        if (!skip_digit()) {
            return invalid_value<string_t>();
        }
    }

    if (*_cur == 'e' || *_cur == 'E') {
        if (++_cur == _end) {
            return invalid_value<string_t>();
        }
        if (*_cur == '+' || *_cur == '-') {
            ++_cur;
        }
        if (!skip_digit()) {
            return invalid_value<string_t>();
        }
    }

    return basic_value<string_t>(basic_value<string_t>::value_type::number, string_t(first, _cur));
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_string()
{
    auto string_opt = parse_stdstring();
    if (!string_opt) {
        return invalid_value<string_t>();
    }
    return basic_value<string_t>(basic_value<string_t>::value_type::string, std::move(string_opt).value());
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_array()
{
    if (*_cur == '[') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    if (!skip_whitespace()) {
        return invalid_value<string_t>();
    }
    else if (*_cur == ']') {
        ++_cur;
        // empty basic_array
        return basic_array<string_t>();
    }

    typename basic_array<string_t>::raw_array result;
    result.reserve(4);
    while (true) {
        if (!skip_whitespace()) {
            return invalid_value<string_t>();
        }

        basic_value<string_t> val = parse_value();

        if (!val.valid() || !skip_whitespace()) {
            return invalid_value<string_t>();
        }

        result.emplace_back(std::move(val));

        if (*_cur == ',') {
            ++_cur;
        }
        else {
            break;
        }
    }

    if (skip_whitespace() && *_cur == ']') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    return basic_array<string_t>(std::move(result));
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE basic_value<string_t> parser<parsing_t, string_t>::parse_object()
{
    if (*_cur == '{') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    if (!skip_whitespace()) {
        return invalid_value<string_t>();
    }
    else if (*_cur == '}') {
        ++_cur;
        // empty basic_object
        return basic_object<string_t>();
    }

    typename basic_object<string_t>::raw_object result;
    result.reserve(4);
    while (true) {
        if (!skip_whitespace()) {
            return invalid_value<string_t>();
        }

        auto key_opt = parse_stdstring();

        if (key_opt && skip_whitespace() && *_cur == ':') {
            ++_cur;
        }
        else {
            return invalid_value<string_t>();
        }

        if (!skip_whitespace()) {
            return invalid_value<string_t>();
        }

        basic_value<string_t> val = parse_value();

        if (!val.valid() || !skip_whitespace()) {
            return invalid_value<string_t>();
        }

        string_t key_escape = escape_string(std::move(key_opt).value());
        result.emplace(std::move(key_escape), std::move(val));

        if (*_cur == ',') {
            ++_cur;
        }
        else {
            break;
        }
    }

    if (skip_whitespace() && *_cur == '}') {
        ++_cur;
    }
    else {
        return invalid_value<string_t>();
    }

    return basic_object<string_t>(std::move(result));
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE std::optional<string_t> parser<parsing_t, string_t>::parse_stdstring()
{
    if (*_cur == '"') {
        ++_cur;
    }
    else {
        return std::nullopt;
    }

    const auto first = _cur;
    auto last = _cur;
    bool is_string_end = false;
    while (!is_string_end && _cur != _end) {
        switch (*_cur) {
        case '\t':
        case '\r':
        case '\n':
            return std::nullopt;
        case '\\': {
            if (++_cur == _end) {
                return std::nullopt;
            }
            switch (*_cur) {
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'u':
                ++_cur;
                break;
            default:
                // Illegal backslash escape
                return std::nullopt;
            }
            break;
        }
        case '"': {
            last = _cur;
            ++_cur;
            is_string_end = true;
            break;
        }
        default:
            ++_cur;
            break;
        }
    }
    if (_cur == _end) {
        return std::nullopt;
    }

    return string_t(first, last);
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE bool parser<parsing_t, string_t>::skip_whitespace() noexcept
{
    while (_cur != _end) {
        switch (*_cur) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            ++_cur;
            break;
        case '\0':
            return false;
        default:
            return true;
        }
    }
    return false;
}

template <typename parsing_t, typename string_t>
MEOJSON_INLINE bool parser<parsing_t, string_t>::skip_digit()
{
    // At least one digit
    if (_cur != _end && std::isdigit(*_cur)) {
        ++_cur;
    }
    else {
        return false;
    }

    while (_cur != _end && std::isdigit(*_cur)) {
        ++_cur;
    }

    if (_cur != _end) {
        return true;
    }
    else {
        return false;
    }
}

// *************************
// *      helper impl         *
// *************************

template <typename string_t>
MEOJSON_INLINE string_t unescape_string(string_t str)
{
    for (size_t pos = 0; pos < str.size(); ++pos) {
        string_t replace_str;
        switch (str[pos]) {
        case '\"':
            replace_str = { '\\', '\"' };
            break;
        case '\\':
            replace_str = { '\\', '\\' };
            break;
        case '\b':
            replace_str = { '\\', 'b' };
            break;
        case '\f':
            replace_str = { '\\', 'f' };
            break;
        case '\n':
            replace_str = { '\\', 'n' };
            break;
        case '\r':
            replace_str = { '\\', 'r' };
            break;
        case '\t':
            replace_str = { '\\', 't' };
            break;
        default:
            continue;
            break;
        }
        str.replace(pos, 1, replace_str);
        ++pos;
    }
    return str;
}

template <typename string_t>
MEOJSON_INLINE string_t escape_string(string_t str)
{
    for (size_t pos = 0; pos + 1 < str.size(); ++pos) {
        if (str[pos] != '\\') {
            continue;
        }
        string_t replace_str;
        switch (str[pos + 1]) {
        case '"':
            replace_str = { '\"' };
            break;
        case '\\':
            replace_str = { '\\' };
            break;
        case 'b':
            replace_str = { '\b' };
            break;
        case 'f':
            replace_str = { '\f' };
            break;
        case 'n':
            replace_str = { '\n' };
            break;
        case 'r':
            replace_str = { '\r' };
            break;
        case 't':
            replace_str = { '\t' };
            break;
        default:
            return string_t();
            break;
        }
        str.replace(pos, 2, replace_str);
    }
    return str;
}
} // namespace json
