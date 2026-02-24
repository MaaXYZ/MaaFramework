#include "PipelineTask.h"

#include <stack>

#include "Component/Recognizer.h"
#include "Controller/ControllerAgent.h"
#include "Global/OptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/PipelineDumper.h"
#include "Resource/PipelineParser.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    if (!context_) {
        LogError << "context is null";
        return false;
    }

    LogFunc << VAR(entry_) << VAR(task_id_);

    std::stack<std::string> jumpback_stack;

    // there is no pretask for the entry, so we use the entry itself
    auto begin_opt = context_->get_pipeline_data(entry_);
    if (!begin_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return false;
    }

    PipelineData node = std::move(*begin_opt);
    std::vector<MAA_RES_NS::NodeAttr> next = { { .name = entry_ } };

    bool error_handling = false;

    while (!next.empty() && !context_->need_to_stop()) {
        cur_node_ = node.name;
        auto node_detail = run_next(next, node);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(node.name);
            return true;
        }

        // 识别命中新节点
        if (node_detail.reco_id != MaaInvalidId) {
            error_handling = false;
            auto hit_opt = context_->get_pipeline_data(node_detail.name);
            if (!hit_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(node_detail.name);
                return false;
            }
            std::string pre_node_name = node.name;
            node = std::move(*hit_opt);

            auto it = std::ranges::find_if(next, [&](const MAA_RES_NS::NodeAttr& n) {
                auto data_opt = context_->get_pipeline_data(n);
                return data_opt && data_opt->name == node_detail.name;
            });
            if (it != next.end() && it->jump_back) {
                LogInfo << "push jumpback_stack:" << pre_node_name;
                jumpback_stack.emplace(pre_node_name);
            }

            if (node_detail.completed) {
                next = node.next;
            }
            else { // 动作执行失败了
                LogWarn << "node not completed, handle error" << VAR(node.name);
                error_handling = true;
                next = node.on_error;
                save_on_error(node.name);
            }
        }
        else if (error_handling) {
            LogError << "error handling loop detected" << VAR(node.name);
            next.clear();
            save_on_error(node.name);
        }
        else {
            LogWarn << "invalid node id, handle error" << VAR(node.name);
            error_handling = true;
            next = node.on_error;
            save_on_error(node.name);
        }

        if (next.empty() && !jumpback_stack.empty()) {
            auto top = std::move(jumpback_stack.top());
            LogInfo << "pop jumpback_stack:" << top;
            jumpback_stack.pop();

            auto top_opt = context_->get_pipeline_data(top);
            if (!top_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(top);
                return false;
            }
            node = std::move(*top_opt);

            next = node.next;
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

NodeDetail PipelineTask::run_next(const std::vector<MAA_RES_NS::NodeAttr>& next, const PipelineData& pretask)
{
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    bool valid = std::ranges::any_of(next, [&](const MAA_RES_NS::NodeAttr& node) {
        auto data_opt = context_->get_pipeline_data(node);
        return data_opt && data_opt->enabled;
    });
    if (!valid) {
        LogInfo << "no valid/enabled node in next" << VAR(next);
        return {};
    }

    auto node_id = generate_node_id();
    const auto start_clock = std::chrono::steady_clock::now();

    auto cur_opt = context_->get_pipeline_data(cur_node_);
    if (!cur_opt) {
        LogError << "get_pipeline_data failed, node not exist" << VAR(cur_node_);
        return {};
    }

    const auto& cur_node = *cur_opt;

    json::value node_cb_detail {
        { "task_id", task_id() },
        { "node_id", node_id },
        { "name", cur_node_ },
        { "focus", cur_node.focus },
    };

    notify(MaaMsg_Node_PipelineNode_Starting, node_cb_detail);

    while (!context_->need_to_stop()) {
        auto current_clock = std::chrono::steady_clock::now();
        cv::Mat image = screencap();

        RecoResult reco = recognize_list(image, next);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(pretask.name);
            break;
        }

        if (!reco.box) {
            // reco_timeout < 0 表示无限等待，跳过超时检查
            if (pretask.reco_timeout >= std::chrono::milliseconds(0) && duration_since(start_clock) > pretask.reco_timeout) {
                LogWarn << "Task timeout" << VAR(pretask.name) << VAR(duration_since(start_clock)) << VAR(pretask.reco_timeout);
                break;
            }

            LogDebug << "sleep_until" << VAR(pretask.rate_limit);
            std::this_thread::sleep_until(current_clock + pretask.rate_limit);

            continue;
        }

        std::string hit_name = reco.name;
        auto hit_opt = context_->get_pipeline_data(hit_name);
        if (!hit_opt) {
            LogError << "get_pipeline_data failed, node not exist" << VAR(hit_name);

            notify(MaaMsg_Node_PipelineNode_Failed, node_cb_detail);

            return {};
        }

        auto act = run_action(reco, *hit_opt);

        // Process anchor settings: empty string means clear
        for (const auto& [anchor, target] : hit_opt->anchor) {
            context_->set_anchor(anchor, target);
        }

        NodeDetail result {
            .node_id = node_id,
            .name = hit_name,
            .reco_id = reco.reco_id,
            .action_id = act.action_id,
            .completed = act.success,
        };

        LogInfo << "PipelineTask node done" << VAR(result) << VAR(task_id_);
        set_node_detail(result.node_id, result);

        node_cb_detail["node_details"] = result;
        node_cb_detail["reco_details"] = reco;
        node_cb_detail["action_details"] = act;

        notify(act.success ? MaaMsg_Node_PipelineNode_Succeeded : MaaMsg_Node_PipelineNode_Failed, node_cb_detail);

        return result;
    }

    NodeDetail result {
        .node_id = node_id,
        .completed = false,
    };
    LogWarn << "PipelineTask bad next" << VAR(result) << VAR(task_id_);
    set_node_detail(result.node_id, result);

    notify(MaaMsg_Node_PipelineNode_Failed, node_cb_detail);

    return result;
}

RecoResult PipelineTask::recognize_list(const cv::Mat& image, const std::vector<MAA_RES_NS::NodeAttr>& list)
{
    LogFunc << VAR(cur_node_) << VAR(list);

    if (!context_) {
        LogError << "context is null";
        return {};
    }

    auto cur_opt = context_->get_pipeline_data(cur_node_);
    if (!cur_opt) {
        LogError << "get_pipeline_data failed, node not exist" << VAR(cur_node_);
        return {};
    }

    const auto& cur_node = *cur_opt;

    const json::value reco_list_cb_detail {
        { "task_id", task_id() },
        { "name", cur_node_ },
        { "list", list },
        { "focus", cur_node.focus },
    };

    notify(MaaMsg_Node_NextList_Starting, reco_list_cb_detail);

    auto batch_plan = prepare_batch_ocr(list);
    auto ocr_cache = batch_plan ? std::make_shared<MAA_VISION_NS::OCRCache>() : nullptr;
    bool batch_triggered = false;

    for (const auto& node : list) {
        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop";
            break;
        }

        auto node_opt = context_->get_pipeline_data(node);
        if (!node_opt) {
            LogError << "get_pipeline_data failed, node not exist" << VAR(node);
            continue;
        }
        const auto& pipeline_data = *node_opt;

        if (batch_plan && !batch_triggered && batch_plan->node_names.contains(pipeline_data.name)) {
            batch_triggered = true;

            Recognizer recognizer(tasker_, *context_, image, ocr_cache);
            recognizer.prefetch_batch_ocr(batch_plan->entries);
        }

        RecoResult result = run_recognition(image, pipeline_data, ocr_cache);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop";
            break;
        }
        if (!result.box) {
            continue;
        }

        notify(MaaMsg_Node_NextList_Succeeded, reco_list_cb_detail);

        return result;
    }

    notify(MaaMsg_Node_NextList_Failed, reco_list_cb_detail);

    return {};
}

std::optional<PipelineTask::BatchOCRPlan> PipelineTask::prepare_batch_ocr(const std::vector<MAA_RES_NS::NodeAttr>& list)
{
    using namespace MAA_RES_NS::Recognition;

    if (!context_) {
        return std::nullopt;
    }

    OCRCollectContext ctx;

    for (const auto& node : list) {
        auto data_opt = context_->get_pipeline_data(node);
        if (!data_opt) {
            continue;
        }
        const auto& data = *data_opt;

        if (!data.enabled) {
            continue;
        }

        size_t current_hit = context_->get_hit_count(data.name);
        if (current_hit >= static_cast<size_t>(data.max_hit)) {
            continue;
        }

        collect_ocr_from_reco(ctx, data.name, data.reco_type, data.reco_param);
    }

    if (ctx.plan.node_names.size() < 2) {
        LogDebug << "batch OCR not needed, eligible OCR nodes < 2" << VAR(ctx.plan.node_names.size());
        return std::nullopt;
    }

    for (const auto& name : ctx.plan.node_names) {
        auto node_opt = context_->get_pipeline_data(name);
        if (!node_opt) {
            continue;
        }
        ctx.plan.entries.emplace_back(
            BatchOCREntry {
                .name = name,
                .param = std::get<MAA_VISION_NS::OCRerParam>(node_opt->reco_param),
            });
    }

    LogInfo << "prepared batch OCR plan" << VAR(ctx.plan.node_names) << VAR(ctx.plan.model);
    return ctx.plan;
}

void PipelineTask::try_add_ocr_node(OCRCollectContext& ctx, const std::string& name, const MAA_VISION_NS::OCRerParam& param)
{
    if (param.roi_target.type == MAA_VISION_NS::TargetType::PreTask) {
        const auto& ref_name = std::get<std::string>(param.roi_target.param);
        if (ctx.plan.node_names.contains(ref_name)) {
            LogDebug << "batch OCR skipping node with PreTask ROI dependency" << VAR(name) << VAR(ref_name);
            return;
        }
    }

    if (param.only_rec) {
        // 这玩意 Batch 出来结果顺序可能是乱的，不知道哪个是哪个
        // 我猜的，没试过，后面有空再看看
        return;
    }

    if (ctx.first) {
        ctx.plan.model = param.model;
        ctx.first = false;
    }
    else if (param.model != ctx.plan.model) {
        LogDebug << "batch OCR skipping node due to model mismatch" << VAR(name) << VAR(param.model) << VAR(ctx.plan.model);
        return;
    }

    ctx.plan.node_names.emplace(name);
}

void PipelineTask::collect_ocr_from_reco(
    OCRCollectContext& ctx,
    const std::string& name,
    MAA_RES_NS::Recognition::Type type,
    const MAA_RES_NS::Recognition::Param& param)
{
    using namespace MAA_RES_NS::Recognition;

    if (type == Type::OCR) {
        try_add_ocr_node(ctx, name, std::get<MAA_VISION_NS::OCRerParam>(param));
    }
    else if (type == Type::And) {
        const auto& and_param = std::get<std::shared_ptr<AndParam>>(param);
        if (!and_param) {
            LogError << "Bad AND param" << VAR(name);
            return;
        }
        collect_ocr_from_sub_recognitions(ctx, and_param->all_of);
    }
    else if (type == Type::Or) {
        const auto& or_param = std::get<std::shared_ptr<OrParam>>(param);
        if (!or_param) {
            LogError << "Bad OR param" << VAR(name);
            return;
        }
        collect_ocr_from_sub_recognitions(ctx, or_param->any_of);
    }
}

void PipelineTask::collect_ocr_from_sub_recognitions(
    OCRCollectContext& ctx,
    const std::vector<MAA_RES_NS::Recognition::SubRecognition>& subs)
{
    using namespace MAA_RES_NS::Recognition;

    for (const auto& sub : subs) {
        if (auto* node_name = std::get_if<std::string>(&sub)) {
            auto sub_opt = context_->get_pipeline_data(*node_name);
            if (!sub_opt) {
                LogError << "Bad sub ref" << VAR(*node_name);
                continue;
            }
            collect_ocr_from_reco(ctx, sub_opt->name, sub_opt->reco_type, sub_opt->reco_param);
        }
        else {
            const auto& inline_sub = std::get<InlineSubRecognition>(sub);
            collect_ocr_from_reco(ctx, inline_sub.sub_name, inline_sub.type, inline_sub.param);
        }
    }
}

void PipelineTask::save_on_error(const std::string& node_name)
{
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    if (!option.save_on_error()) {
        return;
    }

    if (!controller()) {
        LogError << "controller is null";
        return;
    }

    auto image = controller()->cached_image();
    if (image.empty()) {
        LogError << "cached_image is empty";
        return;
    }

    std::string filename = std::format("{}_{}.png", format_now_for_filename(), node_name);
    auto filepath = option.log_dir() / "on_error" / path(filename);
    imwrite(filepath, image);
    LogInfo << "save on error to" << filepath;
}

MAA_TASK_NS_END
