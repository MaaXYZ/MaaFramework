// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

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

    bool query(MaaRecoId reco_id)
    {
        MaaBool hh;
        details::String dd;
        auto res = MaaQueryRecognitionDetail(reco_id, &hh, &hit_box, dd.handle(), draws.handle());
        if (res) {
            hit = !!hh;
            detail = json::parse(dd.str()).value_or(json::object {});
            return true;
        }
        else {
            return false;
        }
    }
};

struct NodeDetail
{
    MaaRecoId reco_id = MaaInvalidId;
    bool success = false;

    bool query(MaaNodeId node_id)
    {
        MaaBool ss;
        auto res = MaaQueryNodeDetail(node_id, &reco_id, &ss);
        if (res) {
            success = !!ss;
            return true;
        }
        else {
            return false;
        }
    }
};

}
