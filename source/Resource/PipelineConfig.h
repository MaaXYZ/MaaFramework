#pragma once

#include "Base/NonCopyable.hpp"

#include <filesystem>

#include <meojson/json.hpp>

#include "Common/MaaConf.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineConfig : public NonCopyable
{
public:
    bool load(const std::filesystem::path& path, bool is_base);

private:
    bool open_and_parse_file(const std::filesystem::path& path);
    bool parse_json(const json::value& json);
    bool parse_task(const std::string& name, const json::value& object);

    template <typename T>
    inline static bool check_type_error(const json::value& json, const std::string& key)
    {
        if (!json.exists(key)) {
            return true;
        }
        return json.at(key).is<T>();
    }

    bool parse_templ_matching_params(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Params& output);
    bool parse_ocr_params(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Params& output);
    bool parse_freezes_wait_params(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Params& output);

    std::unordered_map<std::string, MAA_PIPELINE_RES_NS::Data> data_;
};

MAA_RES_NS_END
