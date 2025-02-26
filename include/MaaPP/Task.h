#pragma once

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "./Exception.h"
#include "./Node.h"
#include "./private/String.h"

namespace maapp
{

struct Task
{
    struct Detail
    {
        std::string entry;
        std::vector<Node> nodes;
        MaaStatus status;
    };

    MaaStatus status() const { return MaaTaskerStatus(tasker_, task_id_); }

    MaaStatus wait() const { return MaaTaskerWait(tasker_, task_id_); }

    Detail detail() const
    {
        Detail result;

        MaaSize count = 0;
        if (!MaaTaskerGetTaskDetail(tasker_, task_id_, nullptr, nullptr, &count, nullptr)) {
            throw FunctionFailed("MaaTaskerGetTaskDetail");
        }

        pri::String entry;
        std::vector<MaaNodeId> nodes(count, 0);

        if (!MaaTaskerGetTaskDetail(tasker_, task_id_, entry.buffer_, nodes.data(), &count, &result.status)) {
            throw FunctionFailed("MaaTaskerGetTaskDetail");
        }
        result.entry = entry;
        result.nodes.reserve(count);
        std::transform(
            nodes.begin(),
            nodes.end(),
            std::back_insert_iterator<std::vector<Node>>(result.nodes),
            [this](MaaNodeId node) -> Node { return { tasker_, node }; });

        return result;
    }

    MaaTasker* tasker_ {};
    MaaTaskId task_id_ {};
};

struct ResTask
{
    MaaStatus status() const { return MaaResourceStatus(resource_, res_id_); }

    MaaStatus wait() const { return MaaResourceWait(resource_, res_id_); }

    MaaResource* resource_ {};
    MaaResId res_id_ {};
};

}
