#include "PatternResMgr.h"

#include <fstream>
#include <regex>
#include <sstream>

#include "MaaUtils/Logger.h"

MAA_RES_NS_BEGIN

bool PatternResMgr::lazy_load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);
    if (!std::filesystem::exists(path)) {
        LogError << "path not exists" << VAR(path);
        return false;
    }

    roots_.emplace_back(path);
    if (std::filesystem::is_directory(path)) {
        return load_dir(path);
    }
    return load_lua_file(path);
}

bool PatternResMgr::load_dir(const std::filesystem::path& path)
{
    bool ret = true;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.path().extension() == ".lua") {
            ret &= load_lua_file(entry.path());
        }
    }
    return ret;
}

bool PatternResMgr::load_lua_file(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        LogError << "failed to open" << VAR(path);
        return false;
    }

    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string content = ss.str();

    // Skip UTF-8 BOM
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF
        && static_cast<unsigned char>(content[1]) == 0xBB
        && static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    // Matches: name = {0xRRGGBB, "dx|dy|0xRRGGBB,...", degree, x1, y1, x2, y2};
    // The name may contain CJK characters and spaces; extra whitespace around '=';
    // the trailing semicolon is optional
    static const std::regex entry_regex(
        R"regex(([^=;{}
]+?)\s*=\s*\{\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*"([^"]*)"\s*,\s*(\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}\s*;?\s*)regex");

    auto begin = std::sregex_iterator(content.begin(), content.end(), entry_regex);
    auto end = std::sregex_iterator();

    int count = 0;
    for (auto it = begin; it != end; ++it) {
        const auto& m = *it;
        std::string name = m[1].str();
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) {
            name.pop_back();
        }
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front()))) {
            name.erase(name.begin());
        }
        if (name.empty()) {
            continue;
        }
        // Skip Lua comment lines
        if (name.starts_with("--")) {
            continue;
        }

        PatternData data;
        try {
            data.base_color = static_cast<int>(std::stoul(m[2].str(), nullptr, 0));
            data.points_str = m[3].str();
            data.degree = std::stoi(m[4].str());
            data.x1 = std::stoi(m[5].str());
            data.y1 = std::stoi(m[6].str());
            data.x2 = std::stoi(m[7].str());
            data.y2 = std::stoi(m[8].str());
        }
        catch (...) {
            LogWarn << "failed to parse entry" << VAR(name);
            continue;
        }

        patterns_[name] = std::move(data);
        ++count;
    }

    LogInfo << VAR(path) << VAR(count);
    return true;
}

void PatternResMgr::clear()
{
    roots_.clear();
    patterns_.clear();
}

const PatternData* PatternResMgr::find(const std::string& name) const
{
    auto it = patterns_.find(name);
    return it != patterns_.end() ? &it->second : nullptr;
}

MAA_RES_NS_END
