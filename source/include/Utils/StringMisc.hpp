#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Ranges.hpp"

MAA_NS_BEGIN

namespace string_detail
{
template <typename StringT>
using sv_type = std::basic_string_view<typename std::remove_reference_t<StringT>::value_type,
                                       typename std::remove_reference_t<StringT>::traits_type>;

template <typename StringT>
using sv_pair = std::pair<sv_type<StringT>, sv_type<StringT>>;
} // namespace string_detail

template <typename StringT, typename CharT = MAA_RNS::ranges::range_value_t<StringT>>
concept IsSomeKindOfString = std::same_as<CharT, char> || std::same_as<CharT, wchar_t>;

template <typename StringArrayT, typename StringT = MAA_RNS::ranges::range_value_t<StringArrayT>>
concept IsSomeKindOfStringArray = IsSomeKindOfString<StringT>;

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline constexpr void string_replace_all_(StringT& str, string_detail::sv_type<StringT> from,
                                          string_detail::sv_type<StringT> to)
{
    for (size_t pos(0);; pos += to.length()) {
        if ((pos = str.find(from, pos)) == StringT::npos) return;
        str.replace(pos, from.length(), to);
    }
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
inline constexpr void string_replace_all_(StringT& str,
                                          std::initializer_list<string_detail::sv_pair<StringT>> replace_pairs)
{
    for (auto&& [from, to] : replace_pairs) {
        string_replace_all_(str, from, to);
    }
}

#ifdef MAA_USE_RANGES_RANGE_V3
// workaround for P2210R2
template <MAA_RNS::ranges::forward_range Rng>
requires(requires(Rng rng) { std::basic_string_view(std::addressof(*rng.begin()), MAA_RNS::ranges::distance(rng)); })
inline auto make_string_view(Rng rng)
{
    return std::basic_string_view(std::addressof(*rng.begin()), MAA_RNS::ranges::distance(rng));
}

template <std::forward_iterator It, std::sized_sentinel_for<It> End>
requires(requires(It beg, End end) { std::basic_string_view(std::addressof(*beg), std::distance(beg, end)); })
inline auto make_string_view(It beg, End end)
{
    return std::basic_string_view(std::addressof(*beg), std::distance(beg, end));
}
#else
template <MAA_RNS::ranges::contiguous_range Rng>
inline auto make_string_view(Rng rng)
{
    return std::basic_string_view(rng.begin(), rng.end());
}

template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
requires(requires(It beg, End end) { std::basic_string_view(beg, end); })
inline auto make_string_view(It beg, End end)
{
    return std::basic_string_view(beg, end);
}
#endif

template <typename StringT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline constexpr auto string_replace_all(StringT&& str, string_detail::sv_type<StringT> from,
                                                       string_detail::sv_type<StringT> to)
{
    std::decay_t<StringT> result = std::forward<StringT>(str);
    string_replace_all_(result, from, to);
    return result;
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline constexpr auto string_replace_all(
    StringT&& str, std::initializer_list<string_detail::sv_pair<StringT>> replace_pairs)
{
    std::decay_t<StringT> result = std::forward<StringT>(str);
    string_replace_all_(result, replace_pairs);
    return result;
}

template <typename StringT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline constexpr auto string_replace_all(const StringT& str,
                                                       const std::map<StringT, StringT>& replace_map)
{
    StringT result = str;
    for (const auto& [from, to] : replace_map) {
        string_replace_all_(result, from, to);
    }
    return result;
}

template <typename StringT, typename CharT = MAA_RNS::ranges::range_value_t<StringT>,
          typename Pred = decltype([](CharT c) -> bool { return c != ' '; })>
requires IsSomeKindOfString<StringT>
inline void string_trim_(StringT& str, Pred not_space = Pred {})
{
    str.erase(MAA_RNS::ranges::find_if(str | MAA_RNS::views::reverse, not_space).base(), str.end());
    str.erase(str.begin(), MAA_RNS::ranges::find_if(str, not_space));
}

template <typename StringT, typename CharT = MAA_RNS::ranges::range_value_t<StringT>>
requires IsSomeKindOfString<StringT>
inline void tolowers_(StringT& str)
{
    MAA_RNS::ranges::for_each(str, [](CharT& ch) -> void { ch = static_cast<CharT>(std::tolower(ch)); });
}

template <typename StringT, typename CharT = MAA_RNS::ranges::range_value_t<StringT>>
requires IsSomeKindOfString<StringT>
inline void touppers_(StringT& str)
{
    MAA_RNS::ranges::for_each(str, [](CharT& ch) -> void { ch = static_cast<CharT>(std::toupper(ch)); });
}

template <typename StringT, typename DelimT>
requires IsSomeKindOfString<StringT>
[[nodiscard]] inline constexpr auto string_split(StringT&& str, DelimT&& delim)
{
    std::vector<std::decay_t<StringT>> result;
    auto views = str | MAA_RNS::views::split(delim) |
                 MAA_RNS::views::transform([](auto&& rng) { return make_string_view(rng); });
    for (auto v : views) {
        result.emplace_back(v);
    }
    return result;
}

MAA_NS_END
