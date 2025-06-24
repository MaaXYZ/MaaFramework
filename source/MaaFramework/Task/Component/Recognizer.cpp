#include "Recognizer.h"

#include "CustomRecognition.h"
#include "Global/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
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

    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    RecoResult result;
    switch (pipeline_data.reco_type) {
    case Type::DirectHit:
        result = direct_hit(pipeline_data.name);
        break;

    case Type::TemplateMatch:
        result = template_match(std::get<TemplateMatcherParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    case Type::FeatureMatch:
        result = feature_match(std::get<FeatureMatcherParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    case Type::ColorMatch:
        result = color_match(std::get<ColorMatcherParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    case Type::OCR:
        result = ocr(std::get<OCRerParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    case Type::NeuralNetworkClassify:
        result = nn_classify(std::get<NeuralNetworkClassifierParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    case Type::NeuralNetworkDetect:
        result = nn_detect(std::get<NeuralNetworkDetectorParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    case Type::Custom:
        result = custom_recognize(std::get<CustomRecognitionParam>(pipeline_data.reco_param), pipeline_data.name);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(pipeline_data.reco_type)) << VAR(pipeline_data.name);
        return {};
    }

    if (debug_mode()) {
        // 图太大了，不能无条件存
        result.raw = image_;
    }
    if (pipeline_data.inverse) {
        LogDebug << "pipeline_data.inverse is true, reverse the result" << VAR(pipeline_data.name) << VAR(result.box);
        result.box = result.box ? std::nullopt : std::make_optional<cv::Rect>();
    }

    tasker_->runtime_cache().set_reco_detail(result.reco_id, result);

    save_draws(pipeline_data.name, result);

    if (result.box) {
        const auto& box = *result.box;
        show_hit_draw(box, pipeline_data.name, result.reco_id);
    }

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

RecoResult Recognizer::direct_hit(const std::string& name)
{
    LogDebug << name;
    return RecoResult {
        .reco_id = MAA_VISION_NS::VisionBase::generate_uid(),
        .name = name,
        .algorithm = "DirectHit",
        .box = cv::Rect {},
    };
}

RecoResult Recognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = resource()->template_res().images(param.template_);
    TemplateMatcher analyzer(image_, roi, param, templs, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
                        .name = name,
                        .algorithm = "TemplateMatch",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = resource()->template_res().images(param.template_);
    FeatureMatcher analyzer(image_, roi, param, templs, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
                        .name = name,
                        .algorithm = "FeatureMatch",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    ColorMatcher analyzer(image_, roi, param, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
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

    cv::Rect roi = get_roi(param.roi_target);

    auto det_session = resource()->ocr_res().deter(param.model);
    auto reco_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer analyzer(image_, roi, param, det_session, reco_session, ocr_session, ocr_cache_, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
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

    cv::Rect roi = get_roi(param.roi_target);

    auto& onnx_res = resource()->onnx_res();
    const auto& session = onnx_res.classifier(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkClassifier analyzer(image_, roi, param, session, mem_info, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
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

    cv::Rect roi = get_roi(param.roi_target);

    auto& onnx_res = resource()->onnx_res();
    const auto& session = onnx_res.detector(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkDetector analyzer(image_, roi, param, session, mem_info, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
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

    if (!tasker_) {
        LogError << "tasker_ is null";
        return {};
    }
    if (!tasker_->resource()) {
        LogError << "resource is null";
        return {};
    }
    cv::Rect roi = get_roi(param.roi_target);

    auto session = tasker_->resource()->custom_recognition(param.name);
    CustomRecognition analyzer(image_, roi, param, session, context_, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .reco_id = analyzer.uid(),
                        .name = name,
                        .algorithm = "Custom",
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

cv::Rect Recognizer::get_roi(const MAA_VISION_NS::Target& roi)
{
    if (!tasker_) {
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
        auto& cache = tasker_->runtime_cache();
        std::string name = std::get<std::string>(roi.param);
        MaaNodeId node_id = cache.get_latest_node(name).value_or(MaaInvalidId);
        NodeDetail node_detail = cache.get_node_detail(node_id).value_or(NodeDetail {});
        RecoResult reco_result = cache.get_reco_result(node_detail.reco_id).value_or(RecoResult {});
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

void Recognizer::save_draws(const std::string& node_name, const RecoResult& result) const
{
    const auto& option = GlobalOptionMgr::get_instance();

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

void Recognizer::show_hit_draw(const cv::Rect& box, const std::string& node_name, MaaRecoId uid) const
{
    if (!GlobalOptionMgr::get_instance().show_hit_draw()) {
        return;
    }

    const std::string kWinName = std::format("Hit: {} {}", node_name, uid);

    cv::Mat draw = image_.clone();

    const cv::Scalar color(0, 255, 0);
    cv::rectangle(draw, box, color, 2);

    cv::imshow(kWinName, draw);
    cv::waitKey(0);
    cv::destroyWindow(kWinName);
}

bool Recognizer::debug_mode() const
{
    return GlobalOptionMgr::get_instance().debug_mode();
}

MAA_RES_NS::ResourceMgr* Recognizer::resource()
{
    return tasker_ ? tasker_->resource() : nullptr;
}

MAA_TASK_NS_END
