#include "PipelineTask.h"

#include <stack>

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    if (!context_) {
        LogError << "context is null";
        return false;
    }

    LogFunc << VAR(entry_);

    std::stack<std::string> task_stack;

    // there is no pretask for the entry, so we use the entry itself
    auto begin_opt = context_->get_pipeline_data(entry_);
    if (!begin_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return false;
    }

    PipelineData node = std::move(*begin_opt);
    PipelineData::NextList next = { entry_ };
    PipelineData::NextList interrupt;
    bool error_handling = false;

    while (!next.empty() && !context_->need_to_stop()) {
        cur_node_ = node.name;

        size_t next_size = next.size();
        PipelineData::NextList list = std::move(next);
        list.insert(list.end(), std::make_move_iterator(interrupt.begin()), std::make_move_iterator(interrupt.end()));

        auto node_detail = run_reco_and_action(list, node);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(node.name);
            return true;
        }

        if (node_detail.completed) {
            error_handling = false;

            // 如果 list 里有同名任务，返回值也一定是第一个。同名任务第一个匹配上了后面肯定也会匹配上（除非 Custom 写了一些什么逻辑）
            // 且 PipelineChecker::check_all_next_list 保证了 next + interrupt 中没有同名任务
            auto pos = std::ranges::find(list, node_detail.name) - list.begin();
            bool is_interrupt = static_cast<size_t>(pos) >= next_size;
            auto hit_opt = context_->get_pipeline_data(node_detail.name);
            if (!hit_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(node_detail.name);
                return false;
            }
            PipelineData hit_node = std::move(*hit_opt);

            if (is_interrupt || hit_node.is_sub) { // for compatibility with v1.x
                LogInfo << "push task_stack:" << node.name;
                task_stack.emplace(node.name);
            }

            node = hit_node;
            next = hit_node.next;
            interrupt = hit_node.interrupt;
        }
        else if (error_handling) {
            LogError << "error handling loop detected" << VAR(node.name);
            next.clear();
            interrupt.clear();
        }
        else {
            LogInfo << "handle error" << VAR(node.name);
            error_handling = true;
            next = node.on_error;
            interrupt.clear();
        }

        if (next.empty() && !task_stack.empty()) {
            auto top = std::move(task_stack.top());
            LogInfo << "pop task_stack:" << top;
            task_stack.pop();

            auto top_opt = context_->get_pipeline_data(top);
            if (!top_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(top);
                return false;
            }
            node = std::move(*top_opt);
            next = node.next;
            interrupt = node.interrupt;
        }
    }

    return !error_handling;
}

void PipelineTask::post_stop()
{
    if (!context_) {
        LogError << "context is null";
        return;
    }
    context_->need_to_stop() = true;
}

NodeDetail PipelineTask::run_reco_and_action(const PipelineData::NextList& list, const PipelineData& pretask)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    RecoResult reco;

    const auto start_clock = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point current_clock;

    while (true) {
        current_clock = std::chrono::steady_clock::now();
        cv::Mat image = screencap();

        reco = run_recognition(image, list);
        if (reco.box) { // hit
            break;
        }

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(pretask.name);
            return {};
        }

        if (duration_since(start_clock) > pretask.reco_timeout) {
            LogError << "Task timeout" << VAR(pretask.name) << VAR(duration_since(start_clock)) << VAR(pretask.reco_timeout) << VAR(list);
            return {};
        }

        LogDebug << "sleep_until" << VAR(pretask.rate_limit);
        std::this_thread::sleep_until(current_clock + pretask.rate_limit);
    }

    auto node_detail = run_action(reco);
    return node_detail;
}

MAA_TASK_NS_END
