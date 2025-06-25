#include "PipelineChecker.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"
#include "Utils/Codec.h"

MAA_RES_NS_BEGIN

bool PipelineChecker::check_all_validity(const PipelineDataMap& data_map)
{
    LogFunc;

    bool ret = check_all_next_list(data_map);
    ret &= check_all_regex(data_map);

    return ret;
}

bool PipelineChecker::check_all_next_list(const PipelineDataMap& data_map)
{
    LogFunc;

    for (const auto& [name, pipeline_data] : data_map) {
        if (!check_next_list(pipeline_data.next, data_map)) {
            LogError << "check_next_list next failed" << VAR(name) << VAR(pipeline_data.next);
            return false;
        }
        if (!check_next_list(pipeline_data.interrupt, data_map)) {
            LogError << "check_next_list interrupt failed" << VAR(name) << VAR(pipeline_data.interrupt);
            return false;
        }
        if (!check_next_list(pipeline_data.on_error, data_map)) {
            LogError << "check_next_list on_error failed" << VAR(name) << VAR(pipeline_data.on_error);
            return false;
        }

        // 这里是由业务逻辑决定了这三个列表不应有重复元素，不代表以后有其他列表也要直接加进来
        std::set<std::string> all_next(pipeline_data.next.begin(), pipeline_data.next.end());
        all_next.insert(pipeline_data.interrupt.begin(), pipeline_data.interrupt.end());
        all_next.insert(pipeline_data.on_error.begin(), pipeline_data.on_error.end());

        if (all_next.size() != pipeline_data.next.size() + pipeline_data.interrupt.size() + pipeline_data.on_error.size()) {
            LogError << "there are duplicate elements in the next, interrupt and on_error" << VAR(name) << VAR(pipeline_data.next)
                     << VAR(pipeline_data.interrupt) << VAR(pipeline_data.on_error);
            return false;
        }
    }
    return true;
}

bool PipelineChecker::check_all_regex(const PipelineDataMap& data_map)
{
    LogFunc;

    for (const auto& [name, pipeline_data] : data_map) {
        if (pipeline_data.reco_type != Recognition::Type::OCR) {
            continue;
        }
        const auto& reco_param = std::get<MAA_VISION_NS::OCRerParam>(pipeline_data.reco_param);
        bool valid = std::ranges::all_of(reco_param.expected, regex_valid)
                     && std::ranges::all_of(reco_param.replace | std::views::keys, regex_valid);
        if (!valid) {
            LogError << "regex invalid" << VAR(name);
            return false;
        }
    }
    return true;
}

bool PipelineChecker::check_next_list(const PipelineData::NextList& next_list, const PipelineDataMap& data_map)
{
    for (const auto& next : next_list) {
        if (!data_map.contains(next)) {
            LogError << "Invalid next node name" << VAR(next);
            return false;
        }
    }
    return true;
}

MAA_RES_NS_END
