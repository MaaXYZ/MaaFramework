#include "Recognizer.h"

#include "CustomRecognition.h"
#include "Global/OptionMgr.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
#include "Vision/ColorMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/NeuralNetworkClassifier.h"
#include "Vision/NeuralNetworkDetector.h"
#include "Vision/OCRer.h"
#include "Vision/TemplateMatcher.h"
#include "Vision/VisionUtils.hpp"

#include <type_traits>

MAA_TASK_NS_BEGIN

Recognizer::Recognizer(Tasker* tasker, Context& context, const cv::Mat& image_, std::shared_ptr<MAA_VISION_NS::OCRCache> ocr_batch_cache)
    : tasker_(tasker)
    , context_(context)
    , image_(image_)
    , sub_filtered_boxes_(std::make_shared<typename decltype(sub_filtered_boxes_)::element_type>())
    , sub_best_box_(std::make_shared<typename decltype(sub_best_box_)::element_type>())
    , ocr_batch_cache_(std::move(ocr_batch_cache))
{
}

Recognizer::Recognizer(const Recognizer& recognizer)
    : tasker_(recognizer.tasker_)
    , context_(recognizer.context_)
    , image_(recognizer.image_)
    // do not copy reco_id_
    , sub_filtered_boxes_(recognizer.sub_filtered_boxes_)
    , sub_best_box_(recognizer.sub_best_box_)
    , ocr_batch_cache_(recognizer.ocr_batch_cache_)
{
}

RecoResult Recognizer::recognize(MAA_RES_NS::Recognition::Type type, const MAA_RES_NS::Recognition::Param& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    if (!tasker_) {
        LogError << "tasker is null";
        return { };
    }

    RecoResult result;

    switch (type) {
    case Type::DirectHit:
        result = direct_hit(std::get<DirectHitParam>(param), name);
        break;

    case Type::TemplateMatch:
        result = template_match(std::get<TemplateMatcherParam>(param), name);
        break;

    case Type::FeatureMatch:
        result = feature_match(std::get<FeatureMatcherParam>(param), name);
        break;

    case Type::ColorMatch:
        result = color_match(std::get<ColorMatcherParam>(param), name);
        break;

    case Type::OCR:
        result = ocr(std::get<OCRerParam>(param), name);
        break;

    case Type::NeuralNetworkClassify:
        result = nn_classify(std::get<NeuralNetworkClassifierParam>(param), name);
        break;

    case Type::NeuralNetworkDetect:
        result = nn_detect(std::get<NeuralNetworkDetectorParam>(param), name);
        break;

    case Type::And:
        result = and_(std::get<std::shared_ptr<AndParam>>(param), name);
        break;

    case Type::Or:
        result = or_(std::get<std::shared_ptr<OrParam>>(param), name);
        break;

    case Type::Custom:
        result = custom_recognize(std::get<CustomRecognitionParam>(param), name);
        break;

    default:
        LogError << "Unknown recognition type" << VAR(static_cast<int>(type)) << VAR(name);
        break;
    }

    if (debug_mode() && !image_.empty()) {
        ImageEncodedBuffer png;
        cv::imencode(".png", image_, png);
        result.raw = std::move(png);
    }

    LogInfo << "reco" << VAR(result);
    auto& rt_cache = tasker_->runtime_cache();
    rt_cache.set_reco_detail(result.reco_id, result);

    save_draws(name, result);

    return result;
}

template <typename Res>
json::value gen_detail(const std::vector<Res>& all, const std::vector<Res>& filtered, const std::optional<Res>& best)
{
    std::ignore = best;

    return {
        { "all", json::array(all) },
        { "filtered", json::array(filtered) },
        { "best", best ? json::value(*best) : json::value(nullptr) },
    };
}

template <typename Res>
std::vector<cv::Rect> get_boxes(const std::vector<Res>& results)
{
    std::vector<cv::Rect> boxes;
    for (const auto& res : results) {
        boxes.emplace_back(res.box);
    }
    return boxes;
}

RecoResult Recognizer::direct_hit(const MAA_VISION_NS::DirectHitParam& param, const std::string& name)
{
    LogDebug << name;

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }

    // DirectHit: 使用第一个 ROI 作为 box
    cv::Rect box = rois.front();

    sub_filtered_boxes_->insert_or_assign(name, rois);
    sub_best_box_->insert_or_assign(name, box);

    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "DirectHit",
        .box = box,
    };
}

template <typename Analyzer>
RecoResult Recognizer::build_result(const std::string& name, const std::string& algorithm, Analyzer&& analyzer)
{
    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect { });

    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = algorithm,
        .box = analyzer.best_result() ? std::make_optional(analyzer.best_result()->box) : std::nullopt,
        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws(),
    };
}

RecoResult Recognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return { };
    }

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }
    auto templs = context_.get_images(param.template_);

    return build_result(name, "TemplateMatch", TemplateMatcher(image_, rois, param, templs, name));
}

RecoResult Recognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return { };
    }

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }
    auto templs = context_.get_images(param.template_);

    return build_result(name, "FeatureMatch", FeatureMatcher(image_, rois, param, templs, name));
}

RecoResult Recognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return { };
    }

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }

    return build_result(name, "ColorMatch", ColorMatcher(image_, rois, param, name));
}

RecoResult Recognizer::ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return { };
    }

    if (!resource()) {
        LogError << "Resource not bound or status is null" << VAR(resource());
        return { };
    }

    std::optional<ColorFilterConfig> color_filter;
    if (!param.color_filter.empty()) {
        auto filter_data = context_.get_pipeline_data(param.color_filter);
        if (!filter_data) {
            LogError << "color_filter node not found" << VAR(param.color_filter);
            return { };
        }
        if (filter_data->reco_type != MAA_RES_NS::Recognition::Type::ColorMatch) {
            LogError << "color_filter node is not ColorMatch" << VAR(param.color_filter);
            return { };
        }
        const auto& color_param = std::get<ColorMatcherParam>(filter_data->reco_param);
        color_filter = ColorFilterConfig { .method = color_param.method, .range = color_param.range };
    }

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }

    if (ocr_batch_cache_ && ocr_batch_cache_->contains(name)) {
        const auto& cached = ocr_batch_cache_->at(name);
        LogDebug << "OCR using batch cache" << VAR(name) << VAR(cached);
        return build_result(name, "OCR", OCRer(image_, rois, param, cached, resource()->ocr_res().recer(param.model), name));
    }

    return build_result(
        name,
        "OCR",
        OCRer(
            image_,
            rois,
            param,
            resource()->ocr_res().deter(param.model),
            resource()->ocr_res().recer(param.model),
            resource()->ocr_res().ocrer(param.model),
            name,
            std::move(color_filter)));
}

RecoResult Recognizer::nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return { };
    }

    if (!resource()) {
        LogError << "Resource not bound";
        return { };
    }

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }

    auto& onnx_res = resource()->onnx_res();

    return build_result(
        name,
        "NeuralNetworkClassify",
        NeuralNetworkClassifier(image_, rois, param, onnx_res.classifier(param.model), onnx_res.memory_info(), name));
}

RecoResult Recognizer::nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return { };
    }

    if (!resource()) {
        LogError << "Resource not bound";
        return { };
    }

    auto rois = get_rois(param.roi_target);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }

    auto& onnx_res = resource()->onnx_res();

    return build_result(
        name,
        "NeuralNetworkDetect",
        NeuralNetworkDetector(image_, rois, param, onnx_res.detector(param.model), onnx_res.memory_info(), name));
}

RecoResult Recognizer::custom_recognize(const MAA_VISION_NS::CustomRecognitionParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "resource is null";
        return { };
    }

    auto rois = get_rois(param.roi_target, true);
    if (rois.empty()) {
        LogWarn << "failed to get rois" << VAR(name);
        return { };
    }

    return build_result(
        name,
        "Custom",
        CustomRecognition(image_, rois.front(), param, resource()->custom_recognition(param.name), context_, name));
}

RecoResult Recognizer::run_sub_recognition(const MAA_RES_NS::Recognition::SubRecognition& sub_reco, const std::string& combinator_name)
{
    using namespace MAA_RES_NS::Recognition;

    Recognizer sub_recognizer(*this);

    if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
        auto node_opt = context_.get_pipeline_data(*node_name);
        if (!node_opt) {
            LogError << combinator_name << "failed to get pipeline data for node" << VAR(*node_name);
            return { };
        }
        LogDebug << combinator_name << "run node reference" << VAR(*node_name);
        return sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
    }

    const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
    LogDebug << combinator_name << "run inline sub recognition" << VAR(inline_sub.type) << VAR(inline_sub.sub_name);
    return sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
}

Recognizer::AndBranchResult Recognizer::match_and_branch(const std::vector<MAA_RES_NS::Recognition::SubRecognition>& all_of, size_t index)
{
    if (index >= all_of.size()) {
        return { .hit = true };
    }

    RecoResult result = run_sub_recognition(all_of[index], "And:");
    register_sub_result_in_cache(result);

    if (!result.box) {
        LogDebug << "And: sub recognition failed";
        return {
            .hit = false,
            .sub_results = { std::move(result) },
        };
    }

    auto sub_filtered_boxes_snapshot = *sub_filtered_boxes_;
    auto sub_best_box_snapshot = *sub_best_box_;

    AndBranchResult first_failed_branch;
    bool has_failed_branch = false;

    for (const cv::Rect& candidate_box : get_candidate_boxes_for_and_branch(all_of, index, result)) {
        *sub_filtered_boxes_ = sub_filtered_boxes_snapshot;
        *sub_best_box_ = sub_best_box_snapshot;

        RecoResult branch_result = result;
        branch_result.box = candidate_box;
        if (!branch_result.name.empty()) {
            sub_filtered_boxes_->insert_or_assign(branch_result.name, std::vector<cv::Rect> { candidate_box });
            sub_best_box_->insert_or_assign(branch_result.name, candidate_box);
        }

        AndBranchResult tail_result = match_and_branch(all_of, index + 1);
        std::vector<RecoResult> branch_results;
        branch_results.reserve(tail_result.sub_results.size() + 1);
        branch_results.emplace_back(std::move(branch_result));
        branch_results.insert(
            branch_results.end(),
            std::make_move_iterator(tail_result.sub_results.begin()),
            std::make_move_iterator(tail_result.sub_results.end()));

        if (tail_result.hit) {
            return {
                .hit = true,
                .sub_results = std::move(branch_results),
            };
        }

        if (!has_failed_branch) {
            first_failed_branch = {
                .hit = false,
                .sub_results = std::move(branch_results),
            };
            has_failed_branch = true;
        }
    }

    *sub_filtered_boxes_ = std::move(sub_filtered_boxes_snapshot);
    *sub_best_box_ = std::move(sub_best_box_snapshot);

    if (has_failed_branch) {
        return first_failed_branch;
    }

    return {
        .hit = false,
        .sub_results = { std::move(result) },
    };
}

std::vector<cv::Rect> Recognizer::get_candidate_boxes_for_and_branch(
    const std::vector<MAA_RES_NS::Recognition::SubRecognition>& all_of,
    size_t index,
    const RecoResult& result) const
{
    if (!result.box) {
        return { };
    }

    if (result.name.empty() || !later_sub_recognitions_use_roi_target(all_of, index + 1, result.name)) {
        return { *result.box };
    }

    auto it = sub_filtered_boxes_->find(result.name);
    if (it == sub_filtered_boxes_->end() || it->second.empty()) {
        return { *result.box };
    }

    return it->second;
}

bool Recognizer::later_sub_recognitions_use_roi_target(
    const std::vector<MAA_RES_NS::Recognition::SubRecognition>& all_of,
    size_t index,
    const std::string& target_name) const
{
    for (size_t i = index; i < all_of.size(); ++i) {
        std::unordered_set<std::string> visited_nodes;
        if (sub_recognition_uses_roi_target(all_of[i], target_name, visited_nodes)) {
            return true;
        }
    }
    return false;
}

bool Recognizer::sub_recognition_uses_roi_target(
    const MAA_RES_NS::Recognition::SubRecognition& sub_reco,
    const std::string& target_name,
    std::unordered_set<std::string>& visited_nodes) const
{
    using namespace MAA_RES_NS::Recognition;

    if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
        if (!visited_nodes.emplace(*node_name).second) {
            return false;
        }

        auto node_opt = context_.get_pipeline_data(*node_name);
        if (!node_opt) {
            return false;
        }
        return recognition_param_uses_roi_target(node_opt->reco_param, target_name, visited_nodes);
    }

    const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
    return recognition_param_uses_roi_target(inline_sub.param, target_name, visited_nodes);
}

bool Recognizer::recognition_param_uses_roi_target(
    const MAA_RES_NS::Recognition::Param& param,
    const std::string& target_name,
    std::unordered_set<std::string>& visited_nodes) const
{
    auto target_uses_name = [&target_name](const MAA_VISION_NS::Target& target) {
        if (target.type != MAA_VISION_NS::Target::Type::PreTask) {
            return false;
        }
        auto* name = std::get_if<std::string>(&target.param);
        return name && *name == target_name;
    };

    return std::visit(
        [&](const auto& value) -> bool {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::shared_ptr<MAA_RES_NS::Recognition::AndParam>>) {
                if (!value) {
                    return false;
                }
                for (const auto& sub : value->all_of) {
                    if (sub_recognition_uses_roi_target(sub, target_name, visited_nodes)) {
                        return true;
                    }
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<MAA_RES_NS::Recognition::OrParam>>) {
                if (!value) {
                    return false;
                }
                for (const auto& sub : value->any_of) {
                    if (sub_recognition_uses_roi_target(sub, target_name, visited_nodes)) {
                        return true;
                    }
                }
                return false;
            }
            else if constexpr (requires { value.roi_target; }) {
                return target_uses_name(value.roi_target);
            }
            else {
                return false;
            }
        },
        param);
}

RecoResult Recognizer::and_(const std::shared_ptr<MAA_RES_NS::Recognition::AndParam>& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;

    if (!param) {
        LogError << "AndParam is null";
        return { };
    }

    LogDebug << "And recognition" << VAR(name) << VAR(param->all_of.size()) << VAR(param->box_index);

    AndBranchResult branch_result = match_and_branch(param->all_of, 0);
    bool all_hit = branch_result.hit;
    std::vector<RecoResult> sub_results = std::move(branch_result.sub_results);

    std::vector<ImageEncodedBuffer> all_draws;
    for (auto& sub : sub_results) {
        all_draws.insert(all_draws.end(), std::make_move_iterator(sub.draws.begin()), std::make_move_iterator(sub.draws.end()));
    }

    RecoResult result {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "And",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!all_hit) {
        LogDebug << "And recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { });
        sub_best_box_->insert_or_assign(name, cv::Rect { });
        return result;
    }

    if (static_cast<int>(sub_results.size()) <= param->box_index) {
        LogError << "all hit, but box_index is out of range" << VAR(name) << VAR(sub_results.size()) << VAR(param->box_index);
        result.box = std::nullopt;
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { });
        sub_best_box_->insert_or_assign(name, cv::Rect { });
        return result;
    }

    result.box = std::move(sub_results[param->box_index].box);

    cv::Rect final_box = result.box.value_or(cv::Rect { });
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

RecoResult Recognizer::or_(const std::shared_ptr<MAA_RES_NS::Recognition::OrParam>& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;

    if (!param) {
        LogError << "OrParam is null";
        return { };
    }

    LogDebug << "Or recognition" << VAR(name) << VAR(param->any_of.size());

    std::vector<RecoResult> sub_results;

    bool has_hit = false;

    for (const auto& sub_reco : param->any_of) {
        RecoResult res = run_sub_recognition(sub_reco, "Or:");

        has_hit = res.box.has_value();
        register_sub_result_in_cache(res);
        sub_results.emplace_back(std::move(res));

        if (has_hit) {
            LogDebug << "Or: sub recognition succeeded";
            break;
        }
    }
    std::vector<ImageEncodedBuffer> all_draws;
    for (auto& sub : sub_results) {
        all_draws.insert(all_draws.end(), std::make_move_iterator(sub.draws.begin()), std::make_move_iterator(sub.draws.end()));
    }

    RecoResult result {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "Or",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!has_hit) {
        LogDebug << "Or recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { });
        sub_best_box_->insert_or_assign(name, cv::Rect { });
        return result;
    }

    if (sub_results.empty()) {
        LogError << "has hit, but no sub results" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { });
        sub_best_box_->insert_or_assign(name, cv::Rect { });
        return { };
    }

    result.box = std::move(sub_results.back().box);

    cv::Rect final_box = result.box.value_or(cv::Rect { });
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

std::vector<cv::Rect> Recognizer::get_rois(const MAA_VISION_NS::Target& roi, bool use_best)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return { };
    }

    using namespace MAA_VISION_NS;

    std::vector<cv::Rect> results;

    switch (roi.type) {
    case Target::Type::Self:
        LogError << "ROI target not support self";
        return { };

    case Target::Type::PreTask:
        results = get_rois_from_pretask(std::get<std::string>(roi.param), use_best);
        break;

    case Target::Type::Anchor: {
        const auto& anchor_name = std::get<std::string>(roi.param);
        auto node_name = context_.get_anchor(anchor_name);
        if (!node_name) {
            LogWarn << "anchor not set" << VAR(anchor_name);
            return { };
        }
        results = get_rois_from_pretask(*node_name, use_best);
    } break;

    case Target::Type::Region:
        results = { std::get<cv::Rect>(roi.param) };
        break;

    default:
        LogError << "Unknown target" << VAR(static_cast<int>(roi.type));
        return { };
    }

    for (cv::Rect& res : results) {
        res.x += roi.offset.x;
        res.y += roi.offset.y;
        res.width += roi.offset.width;
        res.height += roi.offset.height;
    }

    if (results.empty()) {
        return { };
    }

    return MAA_VISION_NS::correct_rois(std::move(results), image_);
}

std::vector<cv::Rect> Recognizer::get_rois_from_pretask(const std::string& name, bool use_best)
{
    if (use_best) {
        if (auto it = sub_best_box_->find(name); it != sub_best_box_->end()) {
            LogDebug << "pre task from sub_best_box_" << VAR(name) << VAR(it->second);
            return { it->second };
        }
    }
    else {
        if (auto it = sub_filtered_boxes_->find(name); it != sub_filtered_boxes_->end()) {
            LogDebug << "pre task from sub_filtered_boxes_" << VAR(name) << VAR(it->second.size());
            return it->second;
        }
    }

    auto& cache = tasker_->runtime_cache();
    auto node_id = cache.get_latest_node(name);
    if (!node_id) {
        LogWarn << "node not found or not executed" << VAR(name);
        return { };
    }
    NodeDetail node_detail = cache.get_node_detail(*node_id).value_or(NodeDetail { });
    RecoResult reco_result = cache.get_reco_result(node_detail.reco_id).value_or(RecoResult { });
    if (!reco_result.box) {
        LogWarn << "node has no recognition box" << VAR(name);
        return { };
    }
    LogDebug << "pre task from cache" << VAR(name) << VAR(*reco_result.box);
    return { *reco_result.box };
}

void Recognizer::save_draws(const std::string& node_name, const RecoResult& result) const
{
    std::string name = std::format("{}_{}", node_name, result.reco_id);
    MAA_VISION_NS::VisionBase::save_draws(name, result.draws);
}

void Recognizer::register_sub_result_in_cache(const RecoResult& res)
{
    if (!res.box.has_value() || res.name.empty()) {
        return;
    }

    auto& cache = tasker_->runtime_cache();
    auto sub_node_id = TaskBase::generate_node_id();
    cache.set_node_detail(sub_node_id, NodeDetail { .node_id = sub_node_id, .name = res.name, .reco_id = res.reco_id, .completed = true });
    cache.set_latest_node(res.name, sub_node_id);
}

bool Recognizer::debug_mode() const
{
    return MAA_GLOBAL_NS::OptionMgr::get_instance().debug_mode();
}

MAA_RES_NS::ResourceMgr* Recognizer::resource()
{
    return tasker_ ? tasker_->resource() : nullptr;
}

void Recognizer::prefetch_batch_ocr(const std::vector<BatchOCREntry>& entries)
{
    // 这个函数虽然叫 batch，最一开始的实现也确实是 gpu batch
    // 但后来发现，直接做 mask 效率更高，于是就走普通 OCR 了
    // 对外接口仍然可以理解为 batch，实际我们的代码实现是 mask

    using namespace MAA_VISION_NS;

    if (!ocr_batch_cache_ || entries.empty() || !resource() || image_.empty()) {
        LogDebug << "prefetch_batch_ocr skipped" << VAR(ocr_batch_cache_) << VAR(entries.empty()) << VAR(resource()) << VAR(image_.empty());
        return;
    }

    cv::Mat masked_image = cv::Mat::zeros(image_.size(), image_.type());
    std::string batch_name;
    cv::Rect union_roi;

    std::unordered_map<std::string, std::vector<cv::Rect>> node_rois;
    for (const auto& entry : entries) {
        auto entry_rois = get_rois(entry.param.roi_target);
        if (entry_rois.empty()) {
            LogWarn << "failed to get rois for batch OCR entry" << VAR(entry.name);
            continue;
        }
        for (const cv::Rect& r : entry_rois) {
            image_(r).copyTo(masked_image(r));

            node_rois[entry.name].emplace_back(r);
            union_roi |= r;
        }
        batch_name += entry.name + "+";
    }

    if (node_rois.empty()) {
        LogWarn << "node_rois is empty" << VAR(entries);
        return;
    }

    OCRerParam batch_param = entries.front().param;

    // 获取所有结果
    batch_param.expected.clear();
    batch_param.threshold = 0;
    batch_param.replace.clear();

    OCRer ocrer(
        masked_image,
        { union_roi },
        batch_param,
        resource()->ocr_res().deter(batch_param.model),
        resource()->ocr_res().recer(batch_param.model),
        resource()->ocr_res().ocrer(batch_param.model),
        batch_name);

    // 这里先把全部沾点边的结果（有交集的）都收集起来，后面实际要用的时候 (OCR::handle_cached) 再进一步划分
    auto intersect = [](const cv::Rect& a, const cv::Rect& b) {
        return (a & b).area() > 0;
    };

    for (const auto& [node, rois] : node_rois) {
        auto& cache = (*ocr_batch_cache_)[node];
        for (const MAA_VISION_NS::OCRerResult& res : ocrer.all_results()) {
            for (const auto& r : rois) {
                if (!intersect(r, res.box)) {
                    continue;
                }
                cache.emplace_back(res);
            }
        }
    }

    LogInfo << "prefetch_batch_ocr completed" << VAR(entries) << VAR(*ocr_batch_cache_);
}

MAA_TASK_NS_END
