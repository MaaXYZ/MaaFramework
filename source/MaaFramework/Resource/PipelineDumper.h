#pragma once

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "PipelineTypes.h"
#include "PipelineTypesV2.h"

MAA_RES_NS_BEGIN

class PipelineDumper
{
public:
    PipelineDumper() = delete;

    static json::object dump(const PipelineData& pp);
    static PipelineV2::JRecognition dump_reco(Recognition::Type type, const Recognition::Param& param);
};

MAA_RES_NS_END
