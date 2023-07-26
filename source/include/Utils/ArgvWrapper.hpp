#pragma once

#include <map>
#include <meojson/json.hpp>

#include "StringMisc.hpp"

MAA_NS_BEGIN

template <typename Argv>
concept CheckArgv = requires(Argv& argv) {
                        argv.clear();
                        argv.reserve(0);
                    };

template <typename Argv>
requires IsSomeKindOfStringArray<Argv> && CheckArgv<Argv>
struct ArgvWrapper
{
    using value = Argv;
    using string = typename Argv::value_type;
    using replacement = typename std::map<string, string>;

    Argv argv;

    bool parse(const json::value& value);
    Argv gen(const std::map<string, string>& replacement) const;
};

template <typename Argv>
requires IsSomeKindOfStringArray<Argv> && CheckArgv<Argv>
bool ArgvWrapper<Argv>::parse(const json::value& value)
{
    if (!value.is_array()) {
        return false;
    }

    const auto& arr = value.as_array();
    if (MAA_RNS::ranges::any_of(arr, [](const json::value& val) { return !val.is_string(); })) {
        return false;
    }

    argv.clear();
    argv.reserve(arr.size());
    MAA_RNS::ranges::transform(arr, std::back_inserter(argv), [](const json::value& val) { return val.as_string(); });
    return true;
}

template <typename Argv>
requires IsSomeKindOfStringArray<Argv> && CheckArgv<Argv>
Argv ArgvWrapper<Argv>::gen(const std::map<string, string>& replacement) const
{
    auto argv_dup = argv;
    for (auto& s : argv_dup) {
        s = string_replace_all(s, replacement);
    }
    return argv_dup;
}

// using _Argv = ArgvWrapper<std::vector<std::string>>;

MAA_NS_END
