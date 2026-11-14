#include "PipelineChecker.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"

#include "PipelineParser.h"

MAA_RES_NS_BEGIN

bool PipelineChecker::check_all_validity(
    const PipelineDataMap& data_map,
    json::array* errors,
    const std::map<std::pair<std::string, std::string>, json::object>* sources)
{
    bool ret = check_all_next_list(data_map, errors, sources);
    ret &= check_all_regex(data_map, errors);

    return ret;
}

bool PipelineChecker::check_all_next_list(
    const PipelineDataMap& data_map,
    json::array* errors,
    const std::map<std::pair<std::string, std::string>, json::object>* sources)
{
    for (const auto& [name, pipeline_data] : data_map) {
        if (!check_next_list(
                pipeline_data.next,
                data_map,
                errors,
                name,
                "next",
                sources && sources->contains({ name, "next" }) ? sources->at({ name, "next" }) : json::object {})) {
            LogError << "check_next_list next failed" << VAR(name) << VAR(pipeline_data.next);
            return false;
        }
        if (!check_next_list(
                pipeline_data.on_error,
                data_map,
                errors,
                name,
                "on_error",
                sources && sources->contains({ name, "on_error" }) ? sources->at({ name, "on_error" }) : json::object {})) {
            LogError << "check_next_list on_error failed" << VAR(name) << VAR(pipeline_data.on_error);
            return false;
        }
    }
    return true;
}

bool PipelineChecker::check_all_regex(const PipelineDataMap& data_map, json::array* errors)
{
    auto is_valid = [](const std::wstring& regex) {
        return regex_valid(regex).has_value();
    };

    for (const auto& [name, pipeline_data] : data_map) {
        if (pipeline_data.reco_type != Recognition::Type::OCR) {
            continue;
        }
        const auto& reco_param = std::get<MAA_VISION_NS::OCRerParam>(pipeline_data.reco_param);
        bool valid =
            std::ranges::all_of(reco_param.expected, is_valid) && std::ranges::all_of(reco_param.replace | std::views::keys, is_valid);
        if (!valid) {
            if (errors) {
                errors->emplace_back(json::object { { "code", "invalid_regex" }, { "node", name } });
            }
            LogError << "regex invalid" << VAR(name);
            return false;
        }
    }
    return true;
}

bool PipelineChecker::check_next_list(
    const std::vector<NodeAttr>& next_list,
    const PipelineDataMap& data_map,
    json::array* errors,
    const std::string& name,
    const char* field,
    const json::object& source)
{
    for (const auto& node : next_list) {
        if (node.anchor) {
            continue;
        }
        if (!data_map.contains(node.name)) {
            if (errors) {
                auto error = source;
                error["code"] = "node_reference_not_found";
                error["node"] = name;
                error["field"] = field;
                error["target"] = node.name;
                errors->emplace_back(std::move(error));
            }
            LogError << "Invalid next node name" << VAR(node.name);
            return false;
        }
    }
    return true;
}

MAA_RES_NS_END
