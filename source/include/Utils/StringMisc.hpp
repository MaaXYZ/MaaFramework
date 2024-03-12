#pragma once

#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Conf/Conf.h"

MAA_NS_BEGIN

template <typename StringT, typename CharT = std::ranges::range_value_t<StringT>>
concept IsSomeKindOfString = std::same_as<CharT, char> || std::same_as<CharT, wchar_t>;

template <typename StringT>
using StringViewT = std::basic_string_view<std::ranges::range_value_t<StringT>>;

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline void string_replace_all_(StringT& str, StringViewT<StringT> from, StringViewT<StringT> to)
{
    for (size_t pos = str.find(from); pos != StringT::npos; pos = str.find(from, pos + to.size())) {
        str.replace(pos, from.size(), to);
    }
}

template <typename StringT, typename MapT>
requires IsSomeKindOfString<StringT>
inline void string_replace_all_(StringT& str, const MapT& replace_map)
{
    // TODO: better algorithm
    for (const auto& [from, to] : replace_map) {
        string_replace_all_(str, from, to);
    }
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline auto
    string_replace_all(StringT&& str, StringViewT<StringT> from, StringViewT<StringT> to)
{
    // TODO: better algorithm
    std::decay_t<StringT> result = std::forward<StringT>(str);
    string_replace_all_(result, from, to);
    return result;
}

template <typename StringT, typename MapT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline auto string_replace_all(StringT&& str, const MapT& replace_map)
{
    // TODO: better algorithm
    std::decay_t<StringT> result = std::forward<StringT>(str);
    string_replace_all_(result, replace_map);
    return result;
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline void string_trim_(StringT& str)
{
    auto not_space = [](auto c) -> bool {
        return c != 32;
    };

    str.erase(std::ranges::find_if(str | std::views::reverse, not_space).base(), str.end());
    str.erase(str.begin(), std::ranges::find_if(str, not_space));
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline void tolowers_(StringT& str)
{
    using CharT = std::ranges::range_value_t<StringT>;
    for (auto& ch : str) {
        ch = static_cast<CharT>(std::tolower(ch));
    }
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline void touppers_(StringT& str)
{
    using CharT = std::ranges::range_value_t<StringT>;
    for (auto& ch : str) {
        ch = static_cast<CharT>(std::toupper(ch));
    }
}

template <typename StringT, typename DelimT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline std::vector<StringT> string_split(const StringT& str, const DelimT& delim)
{
    std::vector<StringT> result;
    auto views = str | std::views::split(delim) | std::views::transform([](auto&& rng) {
                     return std::basic_string_view(rng.begin(), rng.end());
                 });

    for (auto v : views) {
        result.emplace_back(v);
    }
    return result;
}

MAA_NS_END