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
    std::string name;
    bool hit = false;
    MaaRect hit_box = { 0, 0, 0, 0 };
    json::value detail;
    details::Image raw;
    details::ImageList draws;

    void query(MaaRecoId reco_id)
    {
        details::String nn;
        MaaBool hh;
        details::String dd;
        if (!MaaQueryRecognitionDetail(
                reco_id,
                nn.handle(),
                &hh,
                &hit_box,
                dd.handle(),
                raw.handle(),
                draws.handle())) {
            throw FunctionFailed("MaaQueryRecognitionDetail");
        }
        name = nn;
        hit = !!hh;
        detail = json::parse(dd.str()).value_or(json::object {});
    }
};

struct NodeDetail
{
    std::string name;
    MaaRecoId reco_id = MaaInvalidId;
    bool success = false;

    void query(MaaNodeId node_id)
    {
        MaaBool ss;
        details::String nn;
        if (!MaaQueryNodeDetail(node_id, nn.handle(), &reco_id, &ss)) {
            throw FunctionFailed("MaaQueryNodeDetail");
        }
        name = nn;
        success = !!ss;
    }
};

struct TaskDetail
{
    std::string entry;
    std::vector<MaaNodeId> node_ids;

    void query(MaaTaskId id)
    {
        MaaSize size;
        if (!MaaQueryTaskDetail(id, nullptr, nullptr, &size)) {
            throw FunctionFailed("MaaQueryTaskDetail");
        }
        details::String ee;
        std::vector<MaaNodeId> result(size);
        if (!MaaQueryTaskDetail(id, ee.handle(), result.data(), &size)) {
            throw FunctionFailed("MaaQueryTaskDetail");
        }
        entry = ee;
    }
};

}
