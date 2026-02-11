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

MAA_TASK_NS_BEGIN

Recognizer::Recognizer(Tasker* tasker, Context& context, const cv::Mat& image_, std::shared_ptr<MAA_VISION_NS::OCRBatchCache> ocr_batch_cache)
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
        return {};
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

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    // DirectHit: 使用第一个 ROI 作为 box
    cv::Rect box = rois.empty() ? cv::Rect {} : rois.front();

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
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);
    auto templs = context_.get_images(param.template_);

    return build_result(name, "TemplateMatch", TemplateMatcher(image_, rois, param, templs, name));
}

RecoResult Recognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);
    auto templs = context_.get_images(param.template_);

    return build_result(name, "FeatureMatch", FeatureMatcher(image_, rois, param, templs, name));
}

RecoResult Recognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    return build_result(name, "ColorMatch", ColorMatcher(image_, rois, param, name));
}

RecoResult Recognizer::ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (!resource()) {
        LogError << "Resource not bound or status is null" << VAR(resource());
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    if (ocr_batch_cache_ && ocr_batch_cache_->contains(name)) {
        return build_result(name, "OCR", OCRer(image_, rois, param, ocr_batch_cache_->at(name), name));
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
            name));
}

RecoResult Recognizer::nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

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
        return {};
    }

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

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
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target, true);

    return build_result(
        name,
        "Custom",
        CustomRecognition(image_, rois.empty() ? cv::Rect {} : rois.front(), param, resource()->custom_recognition(param.name), context_, name));
}

RecoResult Recognizer::and_(const std::shared_ptr<MAA_RES_NS::Recognition::AndParam>& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;

    if (!param) {
        LogError << "AndParam is null";
        return {};
    }

    LogDebug << "And recognition" << VAR(name) << VAR(param->all_of.size()) << VAR(param->box_index);

    std::vector<RecoResult> sub_results;
    bool all_hit = true;

    for (const auto& sub_reco : param->all_of) {
        Recognizer sub_recognizer(*this);
        RecoResult res;

        if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
            // Resolve node name to get recognition params
            auto node_opt = context_.get_pipeline_data(*node_name);
            if (!node_opt) {
                LogError << "And: failed to get pipeline data for node" << VAR(*node_name);
                all_hit = false;
                break;
            }
            LogDebug << "And: run node reference" << VAR(*node_name);
            res = sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
        }
        else {
            const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
            LogDebug << "And: run inline sub recognition" << VAR(inline_sub.type) << VAR(inline_sub.sub_name);
            res = sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
        }

        all_hit &= res.box.has_value();
        sub_results.emplace_back(std::move(res));

        if (!all_hit) {
            LogDebug << "And: sub recognition failed";
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
        .algorithm = "And",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!all_hit) {
        LogDebug << "And recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (static_cast<int>(sub_results.size()) <= param->box_index) {
        LogError << "all hit, but box_index is out of range" << VAR(name) << VAR(sub_results.size()) << VAR(param->box_index);
        result.box = std::nullopt;
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    result.box = std::move(sub_results[param->box_index].box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
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
        return {};
    }

    LogDebug << "Or recognition" << VAR(name) << VAR(param->any_of.size());

    std::vector<RecoResult> sub_results;

    bool has_hit = false;

    for (const auto& sub_reco : param->any_of) {
        Recognizer sub_recognizer(*this);
        RecoResult res;

        if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
            // Resolve node name to get recognition params
            auto node_opt = context_.get_pipeline_data(*node_name);
            if (!node_opt) {
                LogError << "Or: failed to get pipeline data for node" << VAR(*node_name);
                continue;
            }
            LogDebug << "Or: run node reference" << VAR(*node_name);
            res = sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
        }
        else {
            const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
            LogDebug << "Or: run inline sub recognition" << VAR(inline_sub.type) << VAR(inline_sub.sub_name);
            res = sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
        }

        has_hit = res.box.has_value();
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
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (sub_results.empty()) {
        LogError << "has hit, but no sub results" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return {};
    }

    result.box = std::move(sub_results.back().box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

std::vector<cv::Rect> Recognizer::get_rois(const MAA_VISION_NS::Target& roi, bool use_best)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    using namespace MAA_VISION_NS;

    std::vector<cv::Rect> results;

    switch (roi.type) {
    case Target::Type::Self:
        LogError << "ROI target not support self";
        return {};

    case Target::Type::PreTask:
        results = get_rois_from_pretask(std::get<std::string>(roi.param), use_best);
        break;

    case Target::Type::Region:
        results = { std::get<cv::Rect>(roi.param) };
        break;

    default:
        LogError << "Unknown target" << VAR(static_cast<int>(roi.type));
        return {};
    }

    for (cv::Rect& res : results) {
        res.x += roi.offset.x;
        res.y += roi.offset.y;
        res.width += roi.offset.width;
        res.height += roi.offset.height;
    }
    return results;
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

    // 回退到 runtime_cache
    auto& cache = tasker_->runtime_cache();
    MaaNodeId node_id = cache.get_latest_node(name).value_or(MaaInvalidId);
    NodeDetail node_detail = cache.get_node_detail(node_id).value_or(NodeDetail {});
    RecoResult reco_result = cache.get_reco_result(node_detail.reco_id).value_or(RecoResult {});
    cv::Rect raw = reco_result.box.value_or(cv::Rect {});
    LogDebug << "pre task from cache" << VAR(name) << VAR(raw);
    return { raw };
}

void Recognizer::save_draws(const std::string& node_name, const RecoResult& result) const
{
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    if (!option.save_draw()) {
        return;
    }

    auto dir = option.log_dir() / "vision";

    std::filesystem::create_directories(dir);

    for (const auto& draw : result.draws) {
        std::string filename = std::format("{}_{}_{}.jpg", node_name, result.reco_id, format_now_for_filename());
        auto filepath = dir / path(filename);

        std::ofstream of(filepath, std::ios::out | std::ios::binary);
        of.write(reinterpret_cast<const char*>(draw.data()), draw.size());
        LogDebug << "save draw to" << filepath;
    }
}

bool Recognizer::debug_mode() const
{
    return MAA_GLOBAL_NS::OptionMgr::get_instance().debug_mode();
}

MAA_RES_NS::ResourceMgr* Recognizer::resource()
{
    return tasker_ ? tasker_->resource() : nullptr;
}

void Recognizer::prefetch_batch_ocr(const std::vector<BatchOCREntry>& entries, bool only_rec)
{
    using namespace MAA_VISION_NS;

    if (!ocr_batch_cache_ || entries.empty() || !resource()) {
        return;
    }

    struct RoiInfo
    {
        size_t entry_idx;
        size_t roi_idx;
        cv::Rect roi;
    };

    auto do_roi_merge = [](std::vector<RoiInfo>& infos) {
        struct MergeGroup
        {
            size_t parent_idx;
            std::vector<size_t> child_indices;
        };

        std::ranges::sort(infos, [](const RoiInfo& a, const RoiInfo& b) { return a.roi.area() > b.roi.area(); });

        std::vector<MergeGroup> groups;

        auto contains = [](const cv::Rect& outer, const cv::Rect& inner) {
            return inner.x >= outer.x && inner.y >= outer.y && inner.br().x <= outer.br().x && inner.br().y <= outer.br().y;
        };

        for (size_t i = 0; i < infos.size(); ++i) {
            bool merged = false;
            for (auto& group : groups) {
                if (contains(infos[group.parent_idx].roi, infos[i].roi)) {
                    group.child_indices.push_back(i);
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                groups.push_back({ .parent_idx = i });
            }
        }

        return groups;
    };

    std::vector<std::vector<cv::Rect>> all_rois;
    all_rois.reserve(entries.size());
    for (const auto& entry : entries) {
        all_rois.push_back(get_rois(entry.param.roi_target));
    }

    std::vector<RoiInfo> roi_infos;
    for (size_t ei = 0; ei < entries.size(); ++ei) {
        for (size_t ri = 0; ri < all_rois[ei].size(); ++ri) {
            roi_infos.push_back({ ei, ri, all_rois[ei][ri] });
        }
    }

    if (roi_infos.empty()) {
        return;
    }

    const auto& model = entries.front().param.model;

    if (only_rec) {
        std::vector<cv::Mat> images;
        images.reserve(roi_infos.size());
        for (const auto& info : roi_infos) {
            cv::Rect corrected = correct_roi(info.roi, image_);
            images.push_back(image_(corrected));
        }

        auto flat_results = OCRer::batch_predict_only_rec(images, resource()->ocr_res().recer(model));

        for (size_t i = 0; i < roi_infos.size() && i < flat_results.size(); ++i) {
            const auto& info = roi_infos[i];
            auto& result = flat_results[i];

            cv::Rect corrected = correct_roi(info.roi, image_);
            result.box.x += corrected.x;
            result.box.y += corrected.y;

            auto& cache_val = (*ocr_batch_cache_)[entries[info.entry_idx].name];
            if (cache_val.per_roi_results.size() <= info.roi_idx) {
                cache_val.per_roi_results.resize(info.roi_idx + 1);
            }
            cache_val.per_roi_results[info.roi_idx] = { std::move(result) };
        }
    }
    else {
        auto merge_groups = do_roi_merge(roi_infos);

        std::vector<cv::Mat> images;
        std::vector<cv::Rect> corrected_parent_rois;
        images.reserve(merge_groups.size());
        corrected_parent_rois.reserve(merge_groups.size());

        for (const auto& group : merge_groups) {
            cv::Rect corrected = correct_roi(roi_infos[group.parent_idx].roi, image_);
            corrected_parent_rois.push_back(corrected);
            images.push_back(image_(corrected));
        }

        auto batch_results = OCRer::batch_predict_det_rec(images, resource()->ocr_res().ocrer(model));

        auto store_results = [&](const RoiInfo& info, std::vector<OCRerResult> results) {
            auto& cache_val = (*ocr_batch_cache_)[entries[info.entry_idx].name];
            if (cache_val.per_roi_results.size() <= info.roi_idx) {
                cache_val.per_roi_results.resize(info.roi_idx + 1);
            }
            cache_val.per_roi_results[info.roi_idx] = std::move(results);
        };

        for (size_t gi = 0; gi < merge_groups.size() && gi < batch_results.size(); ++gi) {
            const auto& group = merge_groups[gi];
            auto& parent_results = batch_results[gi];
            const auto& parent_roi = corrected_parent_rois[gi];

            for (auto& res : parent_results) {
                res.box.x += parent_roi.x;
                res.box.y += parent_roi.y;
            }

            store_results(roi_infos[group.parent_idx], parent_results);

            for (size_t child_idx : group.child_indices) {
                const auto& child_info = roi_infos[child_idx];
                cv::Rect child_corrected = correct_roi(child_info.roi, image_);

                std::vector<OCRerResult> child_results;
                for (const auto& res : parent_results) {
                    if (res.box.x >= child_corrected.x && res.box.y >= child_corrected.y
                        && res.box.br().x <= child_corrected.br().x && res.box.br().y <= child_corrected.br().y) {
                        child_results.push_back(res);
                    }
                }
                store_results(child_info, std::move(child_results));
            }
        }
    }

    LogInfo << "prefetch_batch_ocr completed" << VAR(entries.size()) << VAR(roi_infos.size()) << VAR(only_rec);
}

MAA_TASK_NS_END
