#include "RuntimeEvents.h"

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"

namespace
{
struct Capture
{
    MaaTasker* tasker = nullptr;
    std::vector<json::value> events;
    bool stop_on_action = false;
    std::promise<MaaTaskId> stop_result;
    bool child_terminal_queryable = false;
};

void MAA_CALL capture(void*, const char* message, const char* payload, void* arg)
{
    auto& state = *static_cast<Capture*>(arg);
    auto event = json::parse(payload).value();
    event["message"] = message;
    state.events.push_back(event);
    if (std::string_view(message) == MaaMsg_Context_Task_Succeeded) {
        MaaStatus status = MaaStatus_Invalid;
        MaaSize size = 0;
        state.child_terminal_queryable =
            MaaTaskerGetTaskDetail(state.tasker, event.at("task_id").as_integer(), nullptr, nullptr, &size, &status)
            && status == MaaStatus_Succeeded;
    }
    if (state.stop_on_action && std::string_view(message) == MaaMsg_Node_Action_Starting) {
        state.stop_on_action = false;
        state.stop_result.set_value(MaaTaskerPostStop(state.tasker));
    }
}

MaaBool MAA_CALL nested(MaaContext* context, MaaTaskId, const char*, const char*, const char*, MaaRecoId, const MaaRect*, void*)
{
    return MaaContextRunTask(context, "Child", "{}") != MaaInvalidId;
}

const json::value* find(const Capture& state, std::string_view message, std::string_view name = {})
{
    for (const auto& event : state.events) {
        if (event.at("message").as_string() == message && (name.empty() || event.get("name", std::string()) == name)) {
            return &event;
        }
    }
    return nullptr;
}

bool failure(std::string_view stage)
{
    std::cerr << "RuntimeEvents: " << stage << std::endl;
    return false;
}

bool run(MaaTasker* tasker, Capture& state, const char* entry, const char* pipeline)
{
    state.events.clear();
    const bool expects_stop = state.stop_on_action;
    std::future<MaaTaskId> stop_result;
    if (expects_stop) {
        stop_result = state.stop_result.get_future();
    }
    auto id = MaaTaskerPostTask(tasker, entry, pipeline);
    if (id == MaaInvalidId) {
        return failure(std::string("post task failed: ") + entry);
    }
    auto status = MaaTaskerWait(tasker, id);
    if (!expects_stop) {
        return status == MaaStatus_Succeeded || failure(std::string("task failed: ") + entry);
    }

    // post_stop clears the runner and can wake Wait(original_id) before callbacks finish.
    // The callback publishes the stop ID only after post_stop returns; waiting that ID
    // then establishes that the original worker has finished writing all captured events.
    if (stop_result.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        return failure("stop callback did not publish a stop command");
    }
    auto stop_id = stop_result.get();
    if (stop_id == MaaInvalidId || MaaTaskerWait(tasker, stop_id) != MaaStatus_Succeeded) {
        return failure("stop command did not finish successfully");
    }
    MaaSize size = 0;
    if (!MaaTaskerGetTaskDetail(tasker, id, nullptr, nullptr, &size, &status) || status != MaaStatus_Succeeded) {
        return failure("stopped task did not preserve its legacy successful runtime status");
    }
    return true;
}

bool check(MaaTasker* tasker, Capture& state)
{
    if (!run(tasker, state, "Root", R"({
        "Root":{"action":"Custom","custom_action":"nested","pre_delay":0,"post_delay":0},
        "Child":{"pre_delay":0,"post_delay":0}
    })")) {
        return false;
    }
    auto* outer = find(state, MaaMsg_Node_Action_Starting, "Root");
    auto* child = find(state, MaaMsg_Context_Task_Starting);
    auto* child_node = find(state, MaaMsg_Node_PipelineNode_Starting, "Child");
    if (!outer || !child || !child_node || !state.child_terminal_queryable || child->at("context_id") == outer->at("context_id")
        || child->at("parent_task_id") != outer->at("task_id") || child->at("parent").at("kind").as_string() != "action"
        || child->at("parent").at("id") != outer->at("action_id") || child_node->at("task_parent") != child->at("parent")) {
        return failure("nested task parent IDs or terminal cache visibility");
    }

    // Identical successor lists must report the branch actually chosen by the scheduler.
    for (const auto* action : { "Click", "DoNothing" }) {
        json::value pipeline = {
            { "A",
              { { "action", action },
                { "next", json::array { "B" } },
                { "on_error", json::array { "B" } },
                { "pre_delay", 0 },
                { "post_delay", 0 } } },
            { "B", { { "pre_delay", 0 }, { "post_delay", 0 } } },
        };
        if (!run(tasker, state, "A", pipeline.to_string().c_str())) {
            return false;
        }
        bool found = false;
        for (const auto& event : state.events) {
            if (event.at("message").as_string() == MaaMsg_Node_NextList_Succeeded && event.at("name").as_string() == "A"
                && event.at("list_type").as_string() != "entry") {
                auto expected = std::string_view(action) == "Click" ? "on_error" : "next";
                found = event.at("list_type").as_string() == expected && event.at("selected_index").as_integer() == 0
                        && event.at("reco_id").as_integer() != 0 && event.at("node_id").as_integer() != 0
                        && event.at("list_id").as_integer() != 0 && event.at("round").as_integer() == 1;
            }
        }
        if (!found) {
            return failure(std::string("selected candidate identity or branch type: ") + action);
        }
    }

    if (!run(tasker, state, "Jump", R"({
        "Jump":{"next":["[JumpBack]Once","End"],"pre_delay":0,"post_delay":0},
        "Once":{"max_hit":1,"pre_delay":0,"post_delay":0},
        "End":{"pre_delay":0,"post_delay":0}
    })")) {
        return false;
    }
    bool resumed = false;
    for (const auto& event : state.events) {
        if (event.at("message").as_string() == MaaMsg_Node_Transition && event.at("reason").as_string() == "jump_back") {
            resumed = event.at("from").as_string() == "Once" && event.at("to").as_string() == "Jump"
                      && event.at("from_node_id").as_integer() != 0 && event.at("to_node_id").as_integer() != 0;
        }
    }
    if (!resumed) {
        return failure("jump-back resume execution IDs");
    }

    state.stop_on_action = true;
    if (!run(tasker, state, "Stop", R"({"Stop":{"pre_delay":0,"post_delay":0}})")) {
        return false;
    }
    return (find(state, MaaMsg_Node_Action_Cancelled) && find(state, MaaMsg_Node_PipelineNode_Cancelled)
               && !find(state, MaaMsg_Node_Action_Succeeded) && !find(state, MaaMsg_Node_PipelineNode_Failed))
           || failure("cancelled action/node terminal closure");
}
} // namespace

bool runtime_events()
{
    auto* resource = MaaResourceCreate();
    auto* tasker = MaaTaskerCreate();
    Capture state { .tasker = tasker };
    MaaResourceRegisterCustomAction(resource, "nested", nested, nullptr);
    MaaTaskerBindResource(tasker, resource);
    MaaTaskerAddContextSink(tasker, capture, &state);
    bool passed = check(tasker, state);
    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    if (!passed) {
        std::cerr << "RuntimeEvents: execution correlation or cancellation failed" << std::endl;
    }
    return passed;
}
