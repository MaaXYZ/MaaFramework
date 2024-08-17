#include "Recognizer.h"

#include "CustomRecognizer.h"
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
    switch (pipeline_data.rec_type) {
    case Type::DirectHit:
        result = direct_hit(pipeline_data.name);
        break;

    case Type::TemplateMatch:
        result = template_match(std::get<TemplateMatcherParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    case Type::FeatureMatch:
        result = feature_match(std::get<FeatureMatcherParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    case Type::ColorMatch:
        result = color_match(std::get<ColorMatcherParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    case Type::OCR:
        result = ocr(std::get<OCRerParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    case Type::NeuralNetworkClassify:
        result = nn_classify(std::get<NeuralNetworkClassifierParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    case Type::NeuralNetworkDetect:
        result = nn_detect(std::get<NeuralNetworkDetectorParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    case Type::Custom:
        result = custom_recognize(std::get<CustomRecognizerParam>(pipeline_data.rec_param), pipeline_data.name);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(pipeline_data.rec_type)) << VAR(pipeline_data.name);
        return {};
    }

    result.name = pipeline_data.name;

    if (debug_mode()) {
        // 图太大了，不能无条件存
        result.raw = image_;
    }

    save_draws(pipeline_data.name, result);

    tasker_->runtime_cache().add_reco_detail(result.uid, result);

    if (result.box) {
        const auto& box = *result.box;
        show_hit_draw(box, pipeline_data.name, result.uid);
    }

    if (pipeline_data.inverse) {
        LogDebug << "pipeline_data.inverse is true, reverse the result" << VAR(pipeline_data.name) << VAR(result.box.has_value());

        if (result.box) {
            result.box = std::nullopt;
        }
        else {
            result.box = cv::Rect {};
        }
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
    LogTrace << name;
    return RecoResult { .box = cv::Rect {} };
}

RecoResult Recognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    std::vector<std::shared_ptr<cv::Mat>> templates;
    for (const auto& path : param.template_paths) {
        auto templ = resource()->template_res().image(path);
        if (!templ) {
            LogWarn << "Template not found:" << path;
            continue;
        }
        templates.emplace_back(std::move(templ));
    }

    TemplateMatcher analyzer(image_, param, templates, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    std::vector<std::shared_ptr<cv::Mat>> templates;
    for (const auto& path : param.template_paths) {
        auto templ = resource()->template_res().image(path);
        if (!templ) {
            LogWarn << "Template not found:" << path;
            continue;
        }
        templates.emplace_back(std::move(templ));
    }

    FeatureMatcher analyzer(image_, param, templates, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    ColorMatcher analyzer(image_, param, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded or status is null" << VAR(resource());
        return {};
    }

    auto det_session = resource()->ocr_res().deter(param.model);
    auto rec_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer analyzer(image_, param, det_session, rec_session, ocr_session, ocr_cache_, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    auto session = resource()->onnx_res().classifier(param.model);

    NeuralNetworkClassifier analyzer(image_, param, session, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    auto session = resource()->onnx_res().detector(param.model);

    NeuralNetworkDetector analyzer(image_, param, session, name);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

RecoResult Recognizer::custom_recognize(const MAA_VISION_NS::CustomRecognizerParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;
    std::ignore = name; // task name

    if (!tasker_) {
        LogError << "tasker_ is null";
        return {};
    }
    if (!tasker_->resource()) {
        LogError << "resource is null";
        return {};
    }
    auto session = tasker_->resource()->custom_recognizer(param.name);
    CustomRecognizer analyzer(param.name, session, context_, param, image_);

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    return RecoResult { .uid = analyzer.uid(),
                        .box = std::move(box),
                        .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                        .draws = std::move(analyzer).draws() };
}

void Recognizer::save_draws(const std::string& task_name, const RecoResult& result) const
{
    const auto& option = GlobalOptionMgr::get_instance();

    if (!option.save_draw()) {
        return;
    }

    auto dir = option.log_dir() / "vision";

    for (const auto& draw : result.draws) {
        std::string filename = std::format("{}_{}_{}.png", task_name, result.uid, format_now_for_filename());
        auto filepath = dir / path(filename);
        imwrite(filepath, draw);
        LogDebug << "save draw to" << filepath;
    }
}

void Recognizer::show_hit_draw(const cv::Rect& box, const std::string& task_name, MaaRecoId uid) const
{
    if (!GlobalOptionMgr::get_instance().show_hit_draw()) {
        return;
    }

    const std::string kWinName = std::format("Hit: {} {}", task_name, uid);

    cv::Mat draw = image_.clone();

    const cv::Scalar color(0, 255, 0);
    cv::rectangle(draw, box, color, 2);

    cv::imshow(kWinName, draw);
    cv::waitKey(0);
    cv::destroyWindow(kWinName);
}

bool Recognizer::debug_mode() const
{
    return GlobalOptionMgr::get_instance().debug_message();
}

MAA_RES_NS::ResourceMgr* Recognizer::resource()
{
    return tasker_ ? tasker_->resource() : nullptr;
}

MAA_TASK_NS_END
