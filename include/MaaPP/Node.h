#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "./Exception.h"
#include "./Reco.h"
#include "./private/String.h"

namespace maapp
{

struct Node
{
    struct Detail
    {
        std::string name;
        Reco reco;
        bool completed;
    };

    Detail detail() const
    {
        Detail result;

        pri::String name;
        MaaRecoId reco {};
        MaaBool completed {};

        if (!MaaTaskerGetNodeDetail(tasker_, node_id_, name.buffer_, &reco, &completed)) {
            throw FunctionFailed("MaaTaskerGetNodeDetail");
        }

        result.name = name;
        result.reco = { tasker_, reco };
        result.completed = completed;

        return result;
    }

    MaaTasker* tasker_ {};
    MaaNodeId node_id_ {};
};

}
