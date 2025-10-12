#pragma once

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineDumper
{
public:
    PipelineDumper() = delete;

    static json::object dump(const PipelineData& pp);
};

MAA_RES_NS_END
