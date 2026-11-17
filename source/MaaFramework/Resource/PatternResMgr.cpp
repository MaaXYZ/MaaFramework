#include "PatternResMgr.h"

#include <algorithm>
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

    // Stage into a temporary registry first: if any file fails to load,
    // patterns_ is left untouched instead of keeping a partial set from this
    // bundle that a later successful resource commit would validate
    std::unordered_map<std::string, PatternData> staged;
    bool ret = false;
    if (std::filesystem::is_directory(path)) {
        ret = load_dir(path, staged);
    }
    else {
        ret = load_lua_file(path, staged);
    }
    if (!ret) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    roots_.emplace_back(path);
    for (auto& [name, data] : staged) {
        if (patterns_.contains(name)) {
            LogWarn << "duplicate pattern name, overwriting" << VAR(name) << VAR(path);
        }
        patterns_[name] = std::move(data);
    }
    return true;
}

bool PatternResMgr::load_dir(const std::filesystem::path& path, std::unordered_map<std::string, PatternData>& staged)
{
    // Non-throwing construction and increment: permission or filesystem errors
    // are reported as a load failure instead of escaping through AsyncRunner
    std::error_code ec;
    std::filesystem::directory_iterator it(path, ec);
    if (ec) {
        LogError << "failed to enumerate" << VAR(path) << VAR(ec.message());
        return false;
    }

    // Deterministic order: files are loaded in lexicographical order; on
    // duplicate names the later file wins (with a warning)
    std::vector<std::filesystem::path> files;
    for (const auto end_it = std::filesystem::directory_iterator(); it != end_it; it.increment(ec)) {
        if (ec) {
            LogError << "failed to iterate" << VAR(path) << VAR(ec.message());
            return false;
        }
        if (it->path().extension() == ".lua") {
            files.emplace_back(it->path());
        }
    }
    if (ec) {
        LogError << "failed to iterate" << VAR(path) << VAR(ec.message());
        return false;
    }
    std::sort(files.begin(), files.end());

    bool ret = true;
    for (const auto& f : files) {
        ret &= load_lua_file(f, staged);
    }
    return ret;
}

bool PatternResMgr::load_lua_file(const std::filesystem::path& path, std::unordered_map<std::string, PatternData>& staged)
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

        if (staged.contains(name)) {
            LogWarn << "duplicate pattern name, overwriting" << VAR(name) << VAR(path);
        }
        staged[name] = std::move(data);
        ++count;
    }

    LogInfo << VAR(path) << VAR(count);
    return true;
}

void PatternResMgr::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    roots_.clear();
    patterns_.clear();
}

std::optional<PatternData> PatternResMgr::find(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = patterns_.find(name);
    if (it == patterns_.end()) {
        return std::nullopt;
    }
    return it->second;
}

MAA_RES_NS_END
