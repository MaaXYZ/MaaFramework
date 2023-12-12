#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Ranges.hpp"

MAA_NS_BEGIN

template <typename StringT, typename CharT = MAA_RNS::ranges::range_value_t<StringT>>
concept IsSomeKindOfString = std::same_as<CharT, char> || std::same_as<CharT, wchar_t>;

template <typename StringT, typename FromT, typename ToT>
requires IsSomeKindOfString<StringT>
template <typename StringT, typename FromT, typename ToT>
inline StringT string_replace_all_(const StringT& str, const FromT& from, const ToT& to) {
    if (from.empty()) return str;

    StringT n_string;
    n_string.reserve(str.size()); // Reserve to avoid multiple reallocations

    auto from_ptr = std::begin(from);
    for (auto pos = std::begin(str); pos != std::end(str); ++pos) {
        // match from the first char
        if (*pos == *std::begin(from)) {
            auto tmp_pos = pos;
            for (; tmp_pos != std::end(str) && from_ptr != std::end(from); ++tmp_pos, ++from_ptr) {
                if (*tmp_pos != *from_ptr) { break; }
            }
            // if iterated through all char
            if (std::distance(pos, tmp_pos) == std::distance(std::begin(from), std::end(from))) {
                n_string.append(std::begin(to), std::end(to)); // add 'to' new string
                pos = tmp_pos - 1; // skip
            } else {
                n_string += *pos;
            }
            from_ptr = std::begin(from);
        } else {
            n_string += *pos;
        }
    }
    return n_string;
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

template <typename StringT, typename FromT, typename ToT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline auto string_replace_all(StringT&& str, const FromT& from, const ToT& to)
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
    auto not_space = [](auto c) -> bool { return !std::isspace(c); };

    str.erase(MAA_RNS::ranges::find_if(str | MAA_RNS::views::reverse, not_space).base(), str.end());
    str.erase(str.begin(), MAA_RNS::ranges::find_if(str, not_space));
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline void tolowers_(StringT& str)
{
    using CharT = MAA_RNS::ranges::range_value_t<StringT>;
    for (auto& ch : str) {
        ch = static_cast<CharT>(std::tolower(ch));
    }
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline void touppers_(StringT& str)
{
    using CharT = MAA_RNS::ranges::range_value_t<StringT>;
    for (auto& ch : str) {
        ch = static_cast<CharT>(std::toupper(ch));
    }
}

template <typename StringT, typename DelimT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline std::vector<StringT> string_split(const StringT& str, const DelimT& delim)
{
    std::vector<StringT> result;
    auto views = str | MAA_RNS::views::split(delim) |
                 MAA_RNS::views::transform([](auto&& rng) { return std::basic_string_view(rng.begin(), rng.end()); });

    for (auto v : views) {
        result.emplace_back(v);
    }
    return result;
}

MAA_NS_END
