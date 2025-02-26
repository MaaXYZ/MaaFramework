#pragma once

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "./Exception.h"
#include "./Node.h"
#include "./Reco.h"
#include "./Task.h"
#include "./private/String.h"

namespace maapp
{

struct Tasker;

struct Context
{
    Task run_task(const std::string& entry, const std::string& pipeline_override) const
    {
        return {
            MaaContextGetTasker(context_),
            MaaContextRunTask(context_, entry.c_str(), pipeline_override.c_str()),
        };
    }

    Reco run_recognition(const std::string& entry, const std::string& pipeline_override, const std::vector<uint8_t>& image) const
    {
        pri::Image img;
        img = image;

        return {
            MaaContextGetTasker(context_),
            MaaContextRunRecognition(context_, entry.c_str(), pipeline_override.c_str(), img.buffer_),
        };
    }

    Node
        run_action(const std::string& entry, const std::string& pipeline_override, const MaaRect& box, const std::string& reco_detail) const
    {
        return {
            MaaContextGetTasker(context_),
            MaaContextRunAction(context_, entry.c_str(), pipeline_override.c_str(), &box, reco_detail.c_str()),
        };
    }

    void override_pipeline(const std::string& pipeline_override) const
    {
        if (!MaaContextOverridePipeline(context_, pipeline_override.c_str())) {
            throw FunctionFailed("MaaContextOverridePipeline");
        }
    }

    void override_next(const std::string& node_name, const std::vector<std::string>& next_list) const
    {
        pri::StringList nexts;
        nexts = next_list;
        if (!MaaContextOverrideNext(context_, node_name.c_str(), nexts.buffer_)) {
            throw FunctionFailed("MaaContextOverrideNext");
        }
    }

    Task task() const
    {
        return {
            MaaContextGetTasker(context_),
            MaaContextGetTaskId(context_),
        };
    }

    std::shared_ptr<Tasker> tasker() const;

    Context clone() const
    {
        return {
            MaaContextClone(context_),
        };
    }

    MaaContext* context_ {};
};

}
