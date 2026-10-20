#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common/Conf.h"
#include "MaaUtils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

// Pattern data in page.lua format: name = {0xRRGGBB, "dx|dy|0xRRGGBB,...", degree, x1, y1, x2, y2};
struct PatternData
{
    int base_color = 0;      // base point color, 0xRRGGBB
    std::string points_str;  // TSLib-style offset points string
    int degree = 90;         // accuracy, 1~100
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0; // search region (left-top + right-bottom)
};

class PatternResMgr : public NonCopyable
{
public:
    bool lazy_load(const std::filesystem::path& path);
    void clear();

    const PatternData* find(const std::string& name) const;

    // RAII guard that sets the active instance for the current thread while
    // parsing (restores the previous one on destruction)
    class ParseGuard
    {
    public:
        ParseGuard(const PatternResMgr* mgr) : prev_(current_for_parse_) { current_for_parse_ = mgr; }
        ~ParseGuard() { current_for_parse_ = prev_; }
        ParseGuard(const ParseGuard&) = delete;
        ParseGuard& operator=(const ParseGuard&) = delete;

    private:
        const PatternResMgr* prev_;
    };

    // The active instance used by PipelineParser when parsing MultiColorMatch
    // (thread-local so concurrent resource loads don't interfere)
    static const PatternResMgr* current_for_parse() { return current_for_parse_; }

private:
    inline static thread_local const PatternResMgr* current_for_parse_ = nullptr;

private:
    bool load_dir(const std::filesystem::path& path);
    bool load_lua_file(const std::filesystem::path& path);

    std::vector<std::filesystem::path> roots_;
    std::unordered_map<std::string, PatternData> patterns_;
};

MAA_RES_NS_END
