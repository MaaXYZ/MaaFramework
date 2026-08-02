#pragma once

#include <string>
#include <vector>

#include "Common/Conf.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

// A cloud-streaming provider signature. Cloud controllers desugar to Win32:
// a window is located by process + class + title, and driven with the given
// screencap / input methods. Only the game title varies per downstream project.
struct CloudProvider
{
    std::string name;           // provider key used in interface.json, e.g. "geforce_now"
    std::string process_regex;  // owning process image path regex, e.g. "GeForceNOW\\.exe"
    std::string class_regex;    // window class regex, e.g. "CEFCLIENT"
    std::string title_template; // title regex with "{game}" placeholder, e.g. "{game}.*on GeForce NOW"
    std::string screencap;      // Win32 screencap method name, e.g. "PrintWindow"
    std::string input;          // Win32 input method name for mouse + keyboard, e.g. "Seize"
};

// Built-in provider registry. Adding a provider is a single entry here.
inline const CloudProvider* find_cloud_provider(const std::string& name)
{
    static const std::vector<CloudProvider> providers = {
        // GeForce NOW native desktop client (CEF stream window). Signature confirmed
        // empirically by MaaEnd and MaaNTE (class CEFCLIENT via EnumWindows, pid-level;
        // title "<game> ... on GeForce NOW"; PrintWindow + Seize foreground).
        CloudProvider {
            .name = "geforce_now",
            .process_regex = "GeForceNOW\\.exe",
            .class_regex = "CEFCLIENT",
            .title_template = "{game}.*on GeForce NOW",
            .screencap = "PrintWindow",
            .input = "Seize",
        },
    };

    for (const auto& provider : providers) {
        if (provider.name == name) {
            return &provider;
        }
    }
    return nullptr;
}

// Build the effective window-title regex by substituting the game fragment into
// the provider title template. e.g. ("{game}.*on GeForce NOW", "Endfield") ->
// "Endfield.*on GeForce NOW".
inline std::string cloud_window_regex(const CloudProvider& provider, const std::string& game_title)
{
    static const std::string placeholder = "{game}";
    std::string result = provider.title_template;
    for (std::string::size_type pos = result.find(placeholder); pos != std::string::npos; pos = result.find(placeholder, pos)) {
        result.replace(pos, placeholder.size(), game_title);
        pos += game_title.size();
    }
    return result;
}

MAA_PROJECT_INTERFACE_NS_END
