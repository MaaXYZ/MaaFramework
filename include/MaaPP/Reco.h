#pragma once

#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "./Exception.h"
#include "./private/Image.h"
#include "./private/String.h"

namespace maapp
{

struct Reco
{
    struct Detail
    {
        std::string node_name;
        std::string algorithm;
        bool hit;
        MaaRect box;
        std::string detail_json;

        std::vector<uint8_t> raw;
        std::vector<std::vector<uint8_t>> draws;
    };

    Detail detail()
    {
        Detail result;

        pri::String node_name;
        pri::String algorithm;
        MaaBool hit {};
        MaaRect box {};
        pri::String detail_json;
        pri::Image raw;
        pri::ImageList draws;

        if (!MaaTaskerGetRecognitionDetail(
                tasker_,
                reco_id_,
                node_name.buffer_,
                algorithm.buffer_,
                &hit,
                &box,
                detail_json.buffer_,
                raw.buffer_,
                draws.buffer_)) {
            throw FunctionFailed("MaaTaskerGetRecognitionDetail");
        }

        result.node_name = node_name;
        result.algorithm = algorithm;
        result.hit = hit;
        result.box = box;
        result.detail_json = detail_json;
        result.raw = raw;
        result.draws = draws;

        return result;
    }

    MaaTasker* tasker_ {};
    MaaRecoId reco_id_ {};
};

}
