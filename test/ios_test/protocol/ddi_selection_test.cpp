#include "ddi/Selection.h"

#include <cstdio>
#include <limits>
#include <string>

namespace
{
int failures = 0;

void check(bool ok, const char* label)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", label);
    if (!ok) {
        ++failures;
    }
}
}

int main()
{
    namespace ddi = maa::ios::ddi;
    std::string error;
    const auto ipad = ddi::select_for_version("18.7.8", error);
    check(
        ipad && ipad->kind == ddi::Kind::Personalized && ipad->version.major == 18 && ipad->version.minor == 7 && ipad->version.patch == 8
            && ipad->hardware_identity_required && error.empty(),
        "iPadOS 18.7.8 selects Personalized without claiming hardware compatibility");
    const auto classic = ddi::select_for_version("16.7.12", error);
    check(
        classic && classic->kind == ddi::Kind::Classic && !classic->hardware_identity_required,
        "pre-17 selects Classic; actual catalog availability is separate");
    const auto boundary = ddi::select_for_version("17.0", error);
    check(boundary && boundary->kind == ddi::Kind::Personalized, "17.0 is the Personalized category boundary");
    const auto transitional = ddi::select_for_version("26.4", error);
    check(transitional && transitional->kind == ddi::Kind::Personalized, "26.4 does not imply Cryptex capability");
    const auto current = ddi::select_for_version("27.0.1", error);
    check(
        current && current->kind == ddi::Kind::Cryptex && !current->hardware_identity_required,
        "27.0.1 selects the generic Cryptex download category");
    check(!ddi::select_for_version("28.0", error) && !error.empty(), "future major does not silently reuse the current DDI");
    check(!ddi::select_for_version(ddi::ProductVersion { }, error) && !error.empty(), "constructed zero major is rejected");
    const auto major_only = ddi::parse_product_version("18", error);
    check(
        major_only && major_only->major == 18 && major_only->minor == 0 && major_only->patch == 0,
        "major-only numeric ProductVersion is normalized");
    const auto max_component = ddi::parse_product_version("18.4294967295.1", error);
    check(
        max_component && max_component->minor == std::numeric_limits<uint32_t>::max(),
        "numeric parsing preserves an unsigned component without overflow");
    bool invalid_rejected = true;
    for (const char* version : { "",
                                 "0",
                                 "0.1",
                                 "18.",
                                 ".18",
                                 "18..1",
                                 "18.7.8.1",
                                 "18.7beta",
                                 "iPadOS 18.7.8",
                                 " 18.7",
                                 "18.7 ",
                                 "+18.7",
                                 "18.-1",
                                 "4294967296.0",
                                 "18.4294967296",
                                 "18.7.4294967296" }) {
        if (ddi::parse_product_version(version, error) || error.empty()) {
            invalid_rejected = false;
        }
    }
    check(invalid_rejected, "malformed, nonnumeric and overflowing ProductVersion values are rejected");
    bool kinds_roundtrip = true;
    for (const auto kind : { ddi::Kind::Classic, ddi::Kind::Personalized, ddi::Kind::Cryptex }) {
        const auto parsed = ddi::parse_kind(ddi::kind_name(kind), error);
        if (!parsed || *parsed != kind || !error.empty()) {
            kinds_roundtrip = false;
        }
    }
    check(kinds_roundtrip, "all explicit download kinds round trip");
    check(
        !ddi::parse_kind("auto", error) && !error.empty() && !ddi::parse_kind("Cryptex", error),
        "unrecognized explicit kind is rejected");
    check(ddi::select_for_version("18.7.8", error).has_value() && error.empty(), "successful selection clears a previous diagnostic");
    return failures ? 1 : 0;
}
