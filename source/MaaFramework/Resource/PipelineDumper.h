#pragma once

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineDumper
{
public:
    PipelineDumper() = delete;

    static json::object dump(const PipelineData& pp);
    static std::vector<std::string> make_next_raw_list(const std::vector<NodeWithAttr>& nodes);
};

MAA_RES_NS_END
