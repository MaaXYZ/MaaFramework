// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

#include "MaaPP/maa/Exception.hpp"
#include "MaaPP/maa/Image.hpp"
#include "MaaPP/maa/details/String.hpp"

namespace maa
{

struct AnalyzeResult
{
    bool result = false;
    MaaRect rec_box = { 0, 0, 0, 0 };
    std::string rec_detail = "";
};

struct RecognitionDetail
{
    bool hit = false;
    MaaRect hit_box = { 0, 0, 0, 0 };
    json::value detail;
    details::ImageList draws;

    void query(MaaRecoId reco_id)
    {
        MaaBool hh;
        details::String dd;
        if (!MaaQueryRecognitionDetail(reco_id, &hh, &hit_box, dd.handle(), draws.handle())) {
            throw FunctionFailed("MaaQueryRecognitionDetail");
        }
        hit = !!hh;
        detail = json::parse(dd.str()).value_or(json::object {});
    }
};

struct RunningDetail
{
    MaaRecoId reco_id = MaaInvalidId;
    bool success = false;

    void query(MaaRunningId running_id)
    {
        MaaBool ss;
        if (!MaaQueryRunningDetail(running_id, &reco_id, &ss)) {
            throw FunctionFailed("MaaQueryRunningDetail");
        }
        success = !!ss;
    }
};

}
