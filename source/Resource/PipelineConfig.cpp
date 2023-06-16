#include "PipelineConfig.h"

#include "Utils/Logger.hpp"

MAA_RES_NS_BEGIN

using namespace MAA_PIPELINE_RES_NS;

bool PipelineConfig::load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        data_.clear();
    }

    if (!std::filesystem::exists(path)) {
        LogError << "path not exists";
        return false;
    }
    else if (std::filesystem::is_directory(path)) {
        for (auto& entry : std::filesystem::directory_iterator(path)) {
            bool ret = open_and_parse_file(entry.path());
            if (!ret) {
                LogError << "open_and_parse_file failed" << VAR(entry.path());
                return false;
            }
        }
    }
    else if (std::filesystem::is_regular_file(path)) {
        bool ret = open_and_parse_file(path);
        if (!ret) {
            LogError << "open_and_parse_file failed" << VAR(path);
            return false;
        }
    }
    else {
        LogError << "path is not directory or regular file";
        return false;
    }

    return true;
}

bool PipelineConfig::open_and_parse_file(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }

    return parse_json(*json_opt);
}

bool PipelineConfig::parse_json(const json::value& json)
{
    if (!json.is_object()) {
        LogError << "json is not object";
        return false;
    }

    for (const auto& [key, value] : json.as_object()) {
        bool ret = parse_task(key, value);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }
    }
    return true;
}

#ifdef MAA_DEBUG
#define CheckTypeError(T, json, key)                     \
    if (!check_type_error<T>(json, key)) {               \
        LogError << "type error" << VAR(key) << VAR(#T); \
        return false;                                    \
    }
#else
#define CheckTypeError(T, json, key) // do nothing
#endif

bool PipelineConfig::parse_task(const std::string& name, const json::value& object)
{
    Data data;
    data.name = name;

    CheckTypeError(std::string, object, "type");
    std::string rec_type_name = object.get("type", "DirectHit");

    static const std::unordered_map<std::string, Recognition::Type> kRecTypeMap = {
        { "DirectHit", Recognition::Type::DirectHit },     { "TemplateMatch", Recognition::Type::TemplateMatch },
        { "OcrPipeline", Recognition::Type::OcrPipeline }, { "OcrRec", Recognition::Type::OcrRec },
        { "FreezesWait", Recognition::Type::FreezesWait },
    };
    auto rec_type_iter = kRecTypeMap.find(rec_type_name);
    if (rec_type_iter == kRecTypeMap.end()) {
        LogError << "rec type not found" << VAR(rec_type_name);
        return false;
    }
    data.recognition_type = rec_type_iter->second;
    bool rec_param_parsed = false;
    switch (data.recognition_type) {
    case Recognition::Type::DirectHit:
        break;
    case Recognition::Type::TemplateMatch:
        rec_param_parsed = parse_templ_matching_params(object, data.recognition_params);
        break;
    case Recognition::Type::OcrPipeline:
    case Recognition::Type::OcrRec:
        rec_param_parsed = parse_ocr_params(object, data.recognition_params);
        break;
    case Recognition::Type::FreezesWait:
        rec_param_parsed = parse_freezes_wait_params(object, data.recognition_params);
        break;
    default:
        rec_param_parsed = false;
        break;
    }
    if (!rec_param_parsed) {
        LogError << "parse_recognition_params failed" << VAR(rec_type_name);
        return false;
    }

    CheckTypeError(json::array, object, "roi");
    if (auto roi_opt = object.find<json::array>("roi")) {
        auto& roi_aray = *roi_opt;
        if (roi_aray.size() != 4) {
            LogError << "roi size != 4" << VAR(roi_aray.size());
            return false;
        }
        std::vector<int> roi_vec;
        for (const auto& roi : roi_aray) {
            CheckTypeError(int, roi, "roi");
            roi_vec.emplace_back(roi.as_integer());
        }
        data.roi = cv::Rect(roi_vec[0], roi_vec[1], roi_vec[2], roi_vec[3]);
    }

    CheckTypeError(bool, object, "cache");
    data.cache = object.get("cache", false);

    return true;
}

bool PipelineConfig::parse_templ_matching_params(const json::value& input, Recognition::Params& output)
{
    return false;
}

bool PipelineConfig::parse_ocr_params(const json::value& input, Recognition::Params& output)
{
    return false;
}

bool PipelineConfig::parse_freezes_wait_params(const json::value& input, Recognition::Params& output)
{
    return false;
}
#undef CheckTypeError

MAA_RES_NS_END
