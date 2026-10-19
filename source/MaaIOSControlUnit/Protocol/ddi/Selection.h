#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace maa::ios::ddi
{

enum class Kind
{
    Classic,
    Personalized,
    Cryptex
};

struct ProductVersion
{
    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;
};

struct Selection
{
    Kind kind;
    ProductVersion version;
};

// Apple ProductVersion is numeric (e.g. "18.7.8"); a release/build label is not
// accepted. One to three unsigned decimal components are allowed.
std::optional<ProductVersion> parse_product_version(std::string_view text, std::string& error);
std::optional<Kind> parse_kind(std::string_view text, std::string& error);
std::string_view kind_name(Kind kind);

// This selects a category, not a catalog asset or a supported MaaFramework feature.
// Versions below 17 use Classic, 17 through 26 use Personalized, and 27 uses
// Cryptex. A 26.x version alone does not prove Cryptex support. New major
// versions require an explicit policy update instead of reusing the last DDI.
std::optional<Selection> select_for_version(ProductVersion version, std::string& error);
std::optional<Selection> select_for_version(std::string_view version, std::string& error);

} // namespace maa::ios::ddi
