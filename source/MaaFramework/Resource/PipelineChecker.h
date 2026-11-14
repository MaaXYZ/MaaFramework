#pragma once

#include <map>

#include "Common/Conf.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineChecker
{
public:
    PipelineChecker() = delete;

    static bool check_all_validity(
        const PipelineDataMap& data_map,
        json::array* errors = nullptr,
        const std::map<std::pair<std::string, std::string>, json::object>* sources = nullptr);

private:
    static bool check_all_next_list(
        const PipelineDataMap& data_map,
        json::array* errors,
        const std::map<std::pair<std::string, std::string>, json::object>* sources);
    static bool check_all_regex(const PipelineDataMap& data_map, json::array* errors);

    static bool check_next_list(
        const std::vector<NodeAttr>& next_list,
        const PipelineDataMap& data_map,
        json::array* errors,
        const std::string& name,
        const char* field,
        const json::object& source);
};

MAA_RES_NS_END
