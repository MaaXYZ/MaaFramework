#include "ProgressSink.h"

#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <meojson/json.hpp>

#include "MaaFramework/Instance/MaaTasker.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

ProgressSink::ProgressSink(MaaTasker* tasker, int level, std::unordered_map<std::string, std::string> task_names)
    : tasker_(tasker)
    , level_(level)
    , task_names_(std::move(task_names))
{
    if (level_ <= 0) {
        return;
    }
    sink_id_ = MaaTaskerAddSink(tasker_, &ProgressSink::on_event, this);
    if (level_ >= 2) {
        context_sink_id_ = MaaTaskerAddContextSink(tasker_, &ProgressSink::on_event, this);
    }
}

ProgressSink::~ProgressSink()
{
    if (tasker_ && sink_id_ != MaaInvalidId) {
        MaaTaskerRemoveSink(tasker_, sink_id_);
    }
    if (tasker_ && context_sink_id_ != MaaInvalidId) {
        MaaTaskerRemoveContextSink(tasker_, context_sink_id_);
    }
}

std::string ProgressSink::task_label(const std::string& entry) const
{
    auto iter = task_names_.find(entry);
    if (iter == task_names_.end() || iter->second.empty() || iter->second == entry) {
        return MAA_NS::utf8_to_crt(entry);
    }
    return MAA_NS::utf8_to_crt(std::format("{} ({})", iter->second, entry));
}

void ProgressSink::on_event(void* /*handle*/, const char* message, const char* details_json, void* trans_arg)
{
    // 该回调在 MaaFW 工作线程触发并跨 C 边界返回，任何异常逃出都将导致 UB。
    // 集中兜底所有异常，并对入参做最小防御。
    try {
        if (!message || !trans_arg || !details_json) {
            return;
        }
        auto* self = static_cast<ProgressSink*>(trans_arg);

        auto j = json::parse(details_json);
        if (!j) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm {};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        const auto ts = std::format("{:02d}:{:02d}:{:02d}.{:03d}", tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());

        std::string_view msg(message);

        // Task-level events (level >= 1)
        if (msg == MaaMsg_Tasker_Task_Starting) {
            if (auto entry = j->find<std::string>("entry")) {
                std::cout << std::format("[{}][Task] Starting: {}\n", ts, self->task_label(*entry)) << std::flush;
            }
        }
        else if (msg == MaaMsg_Tasker_Task_Succeeded) {
            if (auto entry = j->find<std::string>("entry")) {
                std::cout << std::format("[{}][Task] Succeeded: {}\n", ts, self->task_label(*entry)) << std::flush;
            }
        }
        else if (msg == MaaMsg_Tasker_Task_Failed) {
            if (auto entry = j->find<std::string>("entry")) {
                std::cout << std::format("[{}][Task] Failed: {}\n", ts, self->task_label(*entry)) << std::flush;
            }
        }

        // Node-level events (level >= 2)
        if (self->level_ < 2) {
            return;
        }

        if (msg == MaaMsg_Node_PipelineNode_Starting) {
            if (auto name = j->find<std::string>("name")) {
                std::cout << std::format("  [{}][Node] Running: {}\n", ts, MAA_NS::utf8_to_crt(*name)) << std::flush;
            }
        }
        else if (msg == MaaMsg_Node_PipelineNode_Succeeded) {
            if (auto name = j->find<std::string>("name")) {
                std::cout << std::format("  [{}][Node] Succeeded: {}\n", ts, MAA_NS::utf8_to_crt(*name)) << std::flush;
            }
        }
        else if (msg == MaaMsg_Node_PipelineNode_Failed) {
            if (auto name = j->find<std::string>("name")) {
                std::cout << std::format("  [{}][Node] Failed: {}\n", ts, MAA_NS::utf8_to_crt(*name)) << std::flush;
            }
        }
    }
    catch (const std::exception& e) {
        LogError << "ProgressSink::on_event threw" << VAR(e.what()) << VAR(message);
    }
    catch (...) {
        LogError << "ProgressSink::on_event threw unknown exception" << VAR(message);
    }
}

MAA_PROJECT_INTERFACE_NS_END
