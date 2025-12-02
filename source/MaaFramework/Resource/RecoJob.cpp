#include "RecoJob.h"

#include "Global/OptionMgr.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "ResourceMgr.h"
#include "Task/Component/CustomRecognition.h"
#include "Vision/ColorMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/NeuralNetworkClassifier.h"
#include "Vision/NeuralNetworkDetector.h"
#include "Vision/OCRer.h"
#include "Vision/TemplateMatcher.h"
#include "Vision/VisionUtils.hpp"

MAA_RES_NS_BEGIN

using RecoResult = MAA_TASK_NS::RecoResult;
using NodeDetail = MAA_TASK_NS::NodeDetail;

RecoJob::RecoJob(MaaRecoId reco_id, const PipelineData& pipeline_data, std::shared_ptr<MAA_TASK_NS::Context> context, const cv::Mat& image)
    : reco_id_(reco_id)
    , pipeline_data_(pipeline_data)
    , context_(std::move(context))
    , image_(image)
{
}

void RecoJob::run(MaaResource* res)
{
    resource_ = res;

    if (!resource_) {
        LogError << "resource is null";
        return;
    }

    using namespace Recognition;
    using namespace MAA_VISION_NS;

    RecoResult result;

    switch (pipeline_data_.reco_type) {
    case Type::DirectHit:
        result = direct_hit(pipeline_data_.name);
        break;

    case Type::TemplateMatch:
        result = template_match(std::get<TemplateMatcherParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    case Type::FeatureMatch:
        result = feature_match(std::get<FeatureMatcherParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    case Type::ColorMatch:
        result = color_match(std::get<ColorMatcherParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    case Type::OCR:
        result = ocr(std::get<OCRerParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    case Type::NeuralNetworkClassify:
        result = nn_classify(std::get<NeuralNetworkClassifierParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    case Type::NeuralNetworkDetect:
        result = nn_detect(std::get<NeuralNetworkDetectorParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    case Type::Custom:
        result = custom_recognize(std::get<CustomRecognitionParam>(pipeline_data_.reco_param), pipeline_data_.name);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(pipeline_data_.reco_type)) << VAR(pipeline_data_.name);
        return;
    }

    if (debug_mode()) {
        result.raw = image_;
    }
    if (pipeline_data_.inverse) {
        LogDebug << "pipeline_data_.inverse is true, reverse the result" << VAR(pipeline_data_.name) << VAR(result.box);
        result.box = result.box ? std::nullopt : std::make_optional<cv::Rect>();
    }

    LogInfo << "reco" << VAR(result);
    resource_->reco_cache().set_reco_detail(reco_id_, result);

    save_draws(pipeline_data_.name, result);
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

RecoResult RecoJob::direct_hit(const std::string& name)
{
    LogDebug << name;
    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "DirectHit",
        .box = cv::Rect {},
    };
}

RecoResult RecoJob::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = context_->get_images(param.template_);
    TemplateMatcher analyzer(image_, roi, param, templs, name);

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

RecoResult RecoJob::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = context_->get_images(param.template_);
    FeatureMatcher analyzer(image_, roi, param, templs, name);

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

RecoResult RecoJob::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    ColorMatcher analyzer(image_, roi, param, name);

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

RecoResult RecoJob::ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto det_session = resource_->ocr_res().deter(param.model);
    auto reco_session = resource_->ocr_res().recer(param.model);
    auto ocr_session = resource_->ocr_res().ocrer(param.model);

    OCRer analyzer(image_, roi, param, det_session, reco_session, ocr_session, name);

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

RecoResult RecoJob::nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto& onnx_res = resource_->onnx_res();
    const auto& session = onnx_res.classifier(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkClassifier analyzer(image_, roi, param, session, mem_info, name);

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

RecoResult RecoJob::nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto& onnx_res = resource_->onnx_res();
    const auto& session = onnx_res.detector(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkDetector analyzer(image_, roi, param, session, mem_info, name);

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

RecoResult RecoJob::custom_recognize(const MAA_VISION_NS::CustomRecognitionParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;
    std::ignore = name; // node name

    cv::Rect roi = get_roi(param.roi_target);

    auto session = resource_->custom_recognition(param.name);
    MAA_TASK_NS::CustomRecognition analyzer(image_, roi, param, session, *context_, name);

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

cv::Rect RecoJob::get_roi(const MAA_VISION_NS::Target& roi)
{
    auto* tasker = context_ ? context_->tasker() : nullptr;
    if (!tasker) {
        LogError << "tasker is null";
        return {};
    }

    using namespace MAA_VISION_NS;

    cv::Rect raw {};
    switch (roi.type) {
    case Target::Type::Self:
        LogError << "ROI target not support self";
        return {};

    case Target::Type::PreTask: {
        auto& tasker_cache = tasker->tasker_cache();
        std::string name = std::get<std::string>(roi.param);
        MaaNodeId node_id = tasker_cache.get_latest_node(name).value_or(MaaInvalidId);
        NodeDetail node_detail = tasker_cache.get_node_detail(node_id).value_or(NodeDetail {});
        RecoResult reco_result = resource_->reco_cache().get_reco_result(node_detail.reco_id).value_or(RecoResult {});
        raw = reco_result.box.value_or(cv::Rect {});
        LogDebug << "pre task" << VAR(name) << VAR(raw);
    } break;

    case Target::Type::Region:
        raw = std::get<cv::Rect>(roi.param);
        break;

    default:
        LogError << "Unknown target" << VAR(static_cast<int>(roi.type));
        return {};
    }

    return cv::Rect { raw.x + roi.offset.x, raw.y + roi.offset.y, raw.width + roi.offset.width, raw.height + roi.offset.height };
}

void RecoJob::save_draws(const std::string& node_name, const RecoResult& result) const
{
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    if (!option.save_draw()) {
        return;
    }

    auto dir = option.log_dir() / "vision";

    for (const auto& draw : result.draws) {
        std::string filename = std::format("{}_{}_{}.png", node_name, result.reco_id, format_now_for_filename());
        auto filepath = dir / path(filename);
        imwrite(filepath, draw);
        LogDebug << "save draw to" << filepath;
    }
}

bool RecoJob::debug_mode() const
{
    return MAA_GLOBAL_NS::OptionMgr::get_instance().debug_mode();
}

MAA_RES_NS_END
