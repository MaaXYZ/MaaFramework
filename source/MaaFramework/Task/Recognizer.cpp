#include "Recognizer.h"

#include "Instance/InstanceStatus.h"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.h"
#include "Vision/ColorMatcher.h"
#include "Vision/CustomRecognizer.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/NeuralNetworkClassifier.h"
#include "Vision/NeuralNetworkDetector.h"
#include "Vision/OCRer.h"
#include "Vision/TemplateMatcher.h"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

Recognizer::Recognizer(InstanceInternalAPI* inst)
    : inst_(inst)
{
}

std::optional<Recognizer::Result>
    Recognizer::recognize(const cv::Mat& image, const TaskData& task_data)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    if (!status()) {
        LogError << "Status is null";
        return std::nullopt;
    }

    std::optional<Result> result;
    switch (task_data.rec_type) {
    case Type::DirectHit:
        result = direct_hit(task_data.name);
        break;

    case Type::TemplateMatch:
        result = template_match(
            image,
            std::get<TemplateMatcherParam>(task_data.rec_param),
            task_data.name);
        break;

    case Type::FeatureMatch:
        result = feature_match(
            image,
            std::get<FeatureMatcherParam>(task_data.rec_param),
            task_data.name);
        break;

    case Type::ColorMatch:
        result =
            color_match(image, std::get<ColorMatcherParam>(task_data.rec_param), task_data.name);
        break;

    case Type::OCR:
        result = ocr(image, std::get<OCRerParam>(task_data.rec_param), task_data.name);
        break;

    case Type::NeuralNetworkClassify:
        result = nn_classify(
            image,
            std::get<NeuralNetworkClassifierParam>(task_data.rec_param),
            task_data.name);
        break;

    case Type::NeuralNetworkDetect:
        result = nn_detect(
            image,
            std::get<NeuralNetworkDetectorParam>(task_data.rec_param),
            task_data.name);
        break;

    case Type::Custom:
        result = custom_recognize(
            image,
            std::get<CustomRecognizerParam>(task_data.rec_param),
            task_data.name);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(task_data.rec_type))
                 << VAR(task_data.name);
        return std::nullopt;
    }

    if (result) {
        status()->set_rec_box(task_data.name, result->box);
        status()->set_rec_detail(task_data.name, result->detail);

        show_hit_draw(image, *result, task_data.name);
    }

    if (task_data.inverse) {
        LogDebug << "task_data.inverse is true, reverse the result" << VAR(task_data.name)
                 << VAR(result.has_value());
        return result ? std::nullopt : std::make_optional(Result { .box = cv::Rect() });
    }
    return result;
}

std::optional<Recognizer::Result> Recognizer::direct_hit(const std::string& name)
{
    LogTrace << name;
    return Result { .box = cv::Rect(), .detail = json::array() };
}

std::optional<Recognizer::Result> Recognizer::template_match(
    const cv::Mat& image,
    const MAA_VISION_NS::TemplateMatcherParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
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

    TemplateMatcher matcher(image, param, templates, name);

    auto results = std::move(matcher).filtered_results();
    size_t index = matcher.preferred_index();
    auto draws = std::move(matcher).draws();

    if (index >= results.size()) {
        return std::nullopt;
    }
    const cv::Rect& box = results[index].box;

    return Result { .box = box, .detail = std::move(results), .draws = std::move(draws) };
}

std::optional<Recognizer::Result> Recognizer::feature_match(
    const cv::Mat& image,
    const MAA_VISION_NS::FeatureMatcherParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
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

    FeatureMatcher matcher(image, param, templates, name);

    auto results = std::move(matcher).filtered_results();
    size_t index = matcher.preferred_index();
    auto draws = std::move(matcher).draws();

    if (index >= results.size()) {
        return std::nullopt;
    }
    const cv::Rect& box = results[index].box;

    return Result { .box = box, .detail = std::move(results), .draws = std::move(draws) };
}

std::optional<Recognizer::Result> Recognizer::color_match(
    const cv::Mat& image,
    const MAA_VISION_NS::ColorMatcherParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    ColorMatcher matcher(image, param, name);

    auto results = std::move(matcher).filtered_results();
    size_t index = matcher.preferred_index();
    auto draws = std::move(matcher).draws();

    if (index >= results.size()) {
        return std::nullopt;
    }
    const cv::Rect& box = results[index].box;

    return Result { .box = box, .detail = std::move(results), .draws = std::move(draws) };
}

std::optional<Recognizer::Result> Recognizer::ocr(
    const cv::Mat& image,
    const MAA_VISION_NS::OCRerParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource() || !status()) {
        LogError << "Resource not binded or status is null" << VAR(resource()) << VAR(status());
        return std::nullopt;
    }

    auto det_session = resource()->ocr_res().deter(param.model);
    auto rec_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer ocrer(image, param, det_session, rec_session, ocr_session, status(), name);

    auto results = std::move(ocrer).filtered_results();
    size_t index = ocrer.preferred_index();
    auto draws = std::move(ocrer).draws();

    if (index >= results.size()) {
        return std::nullopt;
    }
    const cv::Rect& box = results[index].box;

    return Result { .box = box, .detail = std::move(results), .draws = std::move(draws) };
}

std::optional<Recognizer::Result> Recognizer::nn_classify(
    const cv::Mat& image,
    const MAA_VISION_NS::NeuralNetworkClassifierParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    auto session = resource()->onnx_res().classifier(param.model);

    NeuralNetworkClassifier classifier(image, param, session, name);

    auto results = std::move(classifier).filtered_results();
    size_t index = classifier.preferred_index();
    auto draws = std::move(classifier).draws();

    if (index >= results.size()) {
        return std::nullopt;
    }
    const cv::Rect& box = results[index].box;

    return Result { .box = box, .detail = std::move(results), .draws = std::move(draws) };
}

std::optional<Recognizer::Result> Recognizer::nn_detect(
    const cv::Mat& image,
    const MAA_VISION_NS::NeuralNetworkDetectorParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    auto session = resource()->onnx_res().detector(param.model);

    NeuralNetworkDetector detector(image, param, session, name);

    auto results = std::move(detector).filtered_results();
    size_t index = detector.preferred_index();
    auto draws = std::move(detector).draws();

    if (index >= results.size()) {
        return std::nullopt;
    }
    const cv::Rect& box = results[index].box;

    return Result { .box = box, .detail = std::move(results), .draws = std::move(draws) };
}

std::optional<Recognizer::Result> Recognizer::custom_recognize(
    const cv::Mat& image,
    const MAA_VISION_NS::CustomRecognizerParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!inst_) {
        LogError << "Instance not binded";
        return std::nullopt;
    }

    auto* session = inst_->custom_recognizer_session(param.name);
    if (!session) {
        LogError << "Custom recognizer not found:" << param.name;
        return std::nullopt;
    }

    CustomRecognizer recognizer(image, param, *session, inst_, name);
    auto results = std::move(recognizer).result();
    bool ret = recognizer.ret();

    if (!ret) {
        return std::nullopt;
    }

    const cv::Rect& box = results.box;
    return Result { .box = box, .detail = std::move(results) };
}

void Recognizer::show_hit_draw(
    const cv::Mat& image,
    const Result& res,
    const std::string& task_name) const
{
    if (!GlobalOptionMgr::get_instance().show_hit_draw()) {
        return;
    }

    const std::string kWinName = std::format("Hit: {}", task_name);

    cv::Mat draw = image.clone();

    const cv::Scalar color(0, 255, 0);
    cv::rectangle(draw, res.box, color, 2);

    cv::imshow(kWinName, draw);
    cv::waitKey(0);
    cv::destroyWindow(kWinName);
}

MAA_TASK_NS_END
