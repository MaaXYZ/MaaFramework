#pragma once

#include <meojson/json.hpp>

#include "Conf/Conf.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

template <auto N>
struct StringLitteral
{
    constexpr StringLitteral(const char (&str)[N]) { std::copy_n(str, N, value); }

    char value[N];
};

template <StringLitteral tag>
struct Tag
{
};

MAA_PROJECT_INTERFACE_NS_END

namespace json::ext
{

template <MAA_PROJECT_INTERFACE_NS::StringLitteral tag>
class jsonization<MAA_PROJECT_INTERFACE_NS::Tag<tag>>
{
public:
    json::value to_json(MAA_PROJECT_INTERFACE_NS::Tag<tag>) const { return tag.value; }

    bool check_json(const json::value& j) const { return j.is_string() && j.to_string() == tag.value; }

    bool from_json(const json::value& j, MAA_PROJECT_INTERFACE_NS::Tag<tag>&) const { return check_json(j); }
};

}
