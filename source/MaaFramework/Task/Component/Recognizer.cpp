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

Recognizer::Recognizer(Tasker* tasker, Context& context, const cv::Mat& image_)
    : tasker_(tasker)
    , context_(context)
    , image_(image_)
{
}

RecoResult Recognizer::recognize(const PipelineData& pipeline_data)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    RecoResult result = run_recognition(pipeline_data.reco_type, pipeline_data.reco_param, pipeline_data.name);

    if (debug_mode()) {
        ImageEncodedBuffer png;
        cv::imencode(".png", image_, png);
        result.raw = std::move(png);
    }

    if (pipeline_data.inverse) {
        LogDebug << "pipeline_data.inverse is true, reverse the result" << VAR(pipeline_data.name) << VAR(result.box);
        result.box = result.box ? std::nullopt : std::make_optional<cv::Rect>();
    }

    LogInfo << "reco" << VAR(result);
    auto& rt_cache = tasker_->runtime_cache();
    rt_cache.set_reco_detail(result.reco_id, result);

    save_draws(pipeline_data.name, result);

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

    sub_filtered_boxes_.insert_or_assign(name, rois);
    sub_best_box_.insert_or_assign(name, box);

    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "DirectHit",
        .box = box,
    };
}

RecoResult Recognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto templs = context_.get_images(param.template_);
    TemplateMatcher analyzer(image_, rois, param, templs, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "TemplateMatch",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto templs = context_.get_images(param.template_);
    FeatureMatcher analyzer(image_, rois, param, templs, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "FeatureMatch",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    ColorMatcher analyzer(image_, rois, param, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "ColorMatch",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not bound or status is null" << VAR(resource());
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto det_session = resource()->ocr_res().deter(param.model);
    auto reco_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer analyzer(image_, rois, param, det_session, reco_session, ocr_session, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "OCR",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto& onnx_res = resource()->onnx_res();
    const auto& session = onnx_res.classifier(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkClassifier analyzer(image_, rois, param, session, mem_info, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "NeuralNetworkClassify",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto& onnx_res = resource()->onnx_res();
    const auto& session = onnx_res.detector(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkDetector analyzer(image_, rois, param, session, mem_info, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "NeuralNetworkDetect",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::custom_recognize(const MAA_VISION_NS::CustomRecognitionParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;
    std::ignore = name; // node name

    if (!resource()) {
        LogError << "resource is null";
        return {};
    }
    std::vector<cv::Rect> rois = get_rois(param.roi_target, true);

    auto session = resource()->custom_recognition(param.name);
    CustomRecognition analyzer(image_, rois.empty() ? cv::Rect {} : rois.front(), param, session, context_, name);

    sub_filtered_boxes_.insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_.insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = reco_id_,
                        .name = name,
                        .algorithm = "Custom",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::and_(const std::shared_ptr<MAA_RES_NS::Recognition::AndParam>& param, const std::string& name)
{
    if (!param) {
        LogError << "AndParam is null";
        return {};
    }

    LogDebug << "And recognition" << VAR(name) << VAR(param->all_of.size()) << VAR(param->box_index);

    std::vector<RecoResult> sub_results;
    bool all_hit = true;

    for (const auto& sub_reco : param->all_of) {
        LogDebug << "And: run sub recognition" << VAR(sub_reco.type) << VAR(sub_reco.sub_name);
        RecoResult res = run_recognition(sub_reco.type, sub_reco.param, sub_reco.sub_name);
        all_hit &= res.box.has_value();

        sub_results.emplace_back(std::move(res));

        if (!all_hit) {
            LogDebug << "And: sub recognition failed at" << VAR(sub_reco.type) << VAR(sub_reco.sub_name);
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
        sub_filtered_boxes_.insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_.insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (static_cast<int>(sub_results.size()) <= param->box_index) {
        LogError << "all hit, but box_index is out of range" << VAR(name) << VAR(sub_results.size()) << VAR(param->box_index);
        result.box = std::nullopt;
        sub_filtered_boxes_.insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_.insert_or_assign(name, cv::Rect {});
        return result;
    }

    result.box = std::move(sub_results[param->box_index].box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_.insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_.insert_or_assign(name, final_box);

    return result;
}

RecoResult Recognizer::or_(const std::shared_ptr<MAA_RES_NS::Recognition::OrParam>& param, const std::string& name)
{
    if (!param) {
        LogError << "OrParam is null";
        return {};
    }

    LogDebug << "Or recognition" << VAR(name) << VAR(param->any_of.size());

    std::vector<RecoResult> sub_results;

    bool has_hit = false;

    for (const auto& sub_reco : param->any_of) {
        LogDebug << "Or: run sub recognition" << VAR(sub_reco.type) << VAR(sub_reco.sub_name);
        RecoResult res = run_recognition(sub_reco.type, sub_reco.param, sub_reco.sub_name);
        has_hit = res.box.has_value();
        sub_results.emplace_back(std::move(res));

        if (has_hit) {
            LogDebug << "Or: sub recognition succeeded at" << VAR(sub_reco.type) << VAR(sub_reco.sub_name);
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
        sub_filtered_boxes_.insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_.insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (sub_results.empty()) {
        LogError << "has hit, but no sub results" << VAR(name);
        sub_filtered_boxes_.insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_.insert_or_assign(name, cv::Rect {});
        return {};
    }

    result.box = std::move(sub_results.back().box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_.insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_.insert_or_assign(name, final_box);

    return result;
}

RecoResult
    Recognizer::run_recognition(MAA_RES_NS::Recognition::Type type, const MAA_RES_NS::Recognition::Param& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    switch (type) {
    case Type::DirectHit:
        return direct_hit(std::get<DirectHitParam>(param), name);

    case Type::TemplateMatch:
        return template_match(std::get<TemplateMatcherParam>(param), name);

    case Type::FeatureMatch:
        return feature_match(std::get<FeatureMatcherParam>(param), name);

    case Type::ColorMatch:
        return color_match(std::get<ColorMatcherParam>(param), name);

    case Type::OCR:
        return ocr(std::get<OCRerParam>(param), name);

    case Type::NeuralNetworkClassify:
        return nn_classify(std::get<NeuralNetworkClassifierParam>(param), name);

    case Type::NeuralNetworkDetect:
        return nn_detect(std::get<NeuralNetworkDetectorParam>(param), name);

    case Type::And:
        return and_(std::get<std::shared_ptr<AndParam>>(param), name);

    case Type::Or:
        return or_(std::get<std::shared_ptr<OrParam>>(param), name);

    case Type::Custom:
        return custom_recognize(std::get<CustomRecognitionParam>(param), name);

    default:
        LogError << "Unknown recognition type" << VAR(static_cast<int>(type)) << VAR(name);
        return {};
    }
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
        if (auto it = sub_best_box_.find(name); it != sub_best_box_.end()) {
            LogDebug << "pre task from sub_best_box_" << VAR(name) << VAR(it->second);
            return { it->second };
        }
    }
    else {
        if (auto it = sub_filtered_boxes_.find(name); it != sub_filtered_boxes_.end()) {
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

MAA_TASK_NS_END
