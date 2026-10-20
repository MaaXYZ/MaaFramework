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

    // The active instance used by PipelineParser when parsing MultiColorMatch
    // (set/cleared by ResourceMgr::load_bundle)
    inline static const PatternResMgr* current_for_parse = nullptr;

private:
    bool load_dir(const std::filesystem::path& path);
    bool load_lua_file(const std::filesystem::path& path);

    std::vector<std::filesystem::path> roots_;
    std::unordered_map<std::string, PatternData> patterns_;
};

MAA_RES_NS_END
