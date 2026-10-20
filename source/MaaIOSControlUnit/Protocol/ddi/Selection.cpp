#include "ddi/Selection.h"

#include <array>
#include <limits>

namespace maa::ios::ddi
{

std::optional<ProductVersion> parse_product_version(std::string_view text, std::string& error)
{
    error.clear();
    std::array<uint32_t, 3> components { };
    size_t count = 0;
    size_t offset = 0;
    while (offset < text.size() && count < components.size()) {
        const size_t begin = offset;
        uint32_t value = 0;
        while (offset < text.size() && text[offset] >= '0' && text[offset] <= '9') {
            const uint32_t digit = static_cast<uint32_t>(text[offset] - '0');
            if (value > (std::numeric_limits<uint32_t>::max() - digit) / 10) {
                error = "Invalid numeric Apple ProductVersion";
                return std::nullopt;
            }
            value = value * 10 + digit;
            ++offset;
        }
        if (offset == begin) {
            error = "Invalid numeric Apple ProductVersion";
            return std::nullopt;
        }
        components[count++] = value;
        if (offset == text.size()) {
            break;
        }
        if (text[offset] != '.' || offset + 1 == text.size() || count == components.size()) {
            error = "Invalid numeric Apple ProductVersion";
            return std::nullopt;
        }
        ++offset;
    }
    if (count == 0 || offset != text.size() || components[0] == 0) {
        error = "Invalid numeric Apple ProductVersion";
        return std::nullopt;
    }
    return ProductVersion { components[0], components[1], components[2] };
}

std::string_view kind_name(Kind kind)
{
    switch (kind) {
    case Kind::Classic:
        return "classic";
    case Kind::Personalized:
        return "personalized";
    case Kind::Cryptex:
        return "cryptex";
    }
    return { };
}

std::optional<Selection> select_for_version(ProductVersion version, std::string& error)
{
    error.clear();
    if (version.major == 0) {
        error = "Invalid numeric Apple ProductVersion";
        return std::nullopt;
    }
    if (version.major > 27) {
        error = "DDI selection is not defined for this future Apple OS major version";
        return std::nullopt;
    }
    if (version.major < 17) {
        return Selection { Kind::Classic, version };
    }
    if (version.major < 27) {
        return Selection { Kind::Personalized, version };
    }
    return Selection { Kind::Cryptex, version };
}

std::optional<Selection> select_for_version(std::string_view text, std::string& error)
{
    const auto version = parse_product_version(text, error);
    if (!version) {
        return std::nullopt;
    }
    return select_for_version(*version, error);
}

} // namespace maa::ios::ddi
