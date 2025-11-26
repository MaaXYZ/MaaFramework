#pragma once

#include "Common/Conf.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineChecker
{
public:
    PipelineChecker() = delete;

    static bool check_all_validity(const PipelineDataMap& data_map);

private:
    static bool check_all_next_list(const PipelineDataMap& data_map);
    static bool check_all_regex(const PipelineDataMap& data_map);

    static bool check_next_list(const std::vector<NodeAttr>& next_list, const PipelineDataMap& data_map);
};

MAA_RES_NS_END
