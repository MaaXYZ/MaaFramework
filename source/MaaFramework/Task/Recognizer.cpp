#include "Recognizer.h"

#include "Instance/InstanceStatus.h"
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

Recognizer::Recognizer(InstanceInternalAPI* inst) : inst_(inst) {}

std::optional<Recognizer::Result> Recognizer::recognize(const cv::Mat& image, const TaskData& task_data)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    if (!status()) {
        LogError << "Status not binded";
        return std::nullopt;
    }

    std::optional<Result> result;
    switch (task_data.rec_type) {
    case Type::DirectHit:
        result = direct_hit(task_data.name);
        break;

    case Type::TemplateMatch:
        result = template_match(image, std::get<TemplateMatcherParam>(task_data.rec_param), task_data.name);
        break;

    case Type::FeatureMatch:
        result = feature_match(image, std::get<FeatureMatcherParam>(task_data.rec_param), task_data.name);
        break;

    case Type::ColorMatch:
        result = color_match(image, std::get<ColorMatcherParam>(task_data.rec_param), task_data.name);
        break;

    case Type::OCR:
        result = ocr(image, std::get<OCRerParam>(task_data.rec_param), task_data.name);
        break;

    case Type::NeuralNetworkClassify:
        result = classify(image, std::get<NeuralNetworkClassifierParam>(task_data.rec_param), task_data.name);
        break;

    case Type::NeuralNetworkDetect:
        result = detect(image, std::get<NeuralNetworkDetectorParam>(task_data.rec_param), task_data.name);
        break;

    case Type::Custom:
        result = custom_recognize(image, std::get<CustomRecognizerParam>(task_data.rec_param), task_data.name);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(task_data.rec_type)) << VAR(task_data.name);
        return std::nullopt;
    }

    if (result) {
        status()->set_rec_box(task_data.name, result->box);
        status()->set_rec_detail(task_data.name, result->detail);
    }

    if (task_data.inverse) {
        LogDebug << "task_data.inverse is true, reverse the result" << VAR(task_data.name) << VAR(result.has_value());
        return result ? std::nullopt : std::make_optional(Result { .box = cv::Rect() });
    }
    return result;
}

std::optional<Recognizer::Result> Recognizer::direct_hit(const std::string& name)
{
    LogDebug << name;
    return Result { .box = cv::Rect(), .detail = json::array() };
}

std::optional<Recognizer::Result> Recognizer::template_match(const cv::Mat& image,
                                                             const MAA_VISION_NS::TemplateMatcherParam& param,
                                                             const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    TemplateMatcher matcher;
    matcher.set_image(image);
    matcher.set_name(name);
    matcher.set_param(param);

    std::vector<std::shared_ptr<cv::Mat>> templates;
    for (const auto& path : param.template_paths) {
        auto templ = resource()->template_res().image(path);
        if (!templ) {
            LogWarn << "Template not found:" << path;
            continue;
        }
        templates.emplace_back(std::move(templ));
    }
    matcher.set_templates(std::move(templates));

    auto results = matcher.analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = results.front().box;
    auto detail = json::serialize<false>(results);
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::feature_match(const cv::Mat& image,
                                                            const MAA_VISION_NS::FeatureMatcherParam& param,
                                                            const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    FeatureMatcher matcher;
    matcher.set_image(image);
    matcher.set_name(name);
    matcher.set_param(param);

    std::shared_ptr<cv::Mat> templ = resource()->template_res().image(param.template_path);
    matcher.set_template(std::move(templ));

    auto results = matcher.analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = results.front().box;
    auto detail = json::serialize<false>(results);
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::color_match(const cv::Mat& image,
                                                          const MAA_VISION_NS::ColorMatcherParam& param,
                                                          const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    ColorMatcher matcher;
    matcher.set_image(image);
    matcher.set_name(name);
    matcher.set_param(param);

    auto results = matcher.analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = results.front().box;
    auto detail = json::serialize<false>(results);
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::ocr(const cv::Mat& image, const MAA_VISION_NS::OCRerParam& param,
                                                  const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    OCRer ocrer;
    ocrer.set_image(image);
    ocrer.set_name(name);
    ocrer.set_param(param);

    auto det_session = resource()->ocr_res().deter(param.model);
    auto rec_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);
    ocrer.set_session(std::move(det_session), std::move(rec_session), std::move(ocr_session));

    auto results = ocrer.analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    // TODO: sort by required regex.
    // sort_by_required_(res, param.text);

    const cv::Rect& box = results.front().box;
    auto detail = json::serialize<false>(results);
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::classify(const cv::Mat& image,
                                                       const MAA_VISION_NS::NeuralNetworkClassifierParam& param,
                                                       const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    NeuralNetworkClassifier classifier;
    classifier.set_image(image);
    classifier.set_name(name);
    classifier.set_param(param);

    auto session = resource()->onnx_res().classifier(param.model);
    classifier.set_session(std::move(session));

    auto results = classifier.analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = results.front().box;
    json::array detail;
    for (const auto& res : results) {
        detail.emplace_back(res);
    }
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::detect(const cv::Mat& image,
                                                     const MAA_VISION_NS::NeuralNetworkDetectorParam& param,
                                                     const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    NeuralNetworkDetector detector;
    detector.set_image(image);
    detector.set_name(name);
    detector.set_param(param);

    auto session = resource()->onnx_res().detector(param.model);
    detector.set_session(std::move(session));

    auto results = detector.analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = results.front().box;
    auto detail = json::serialize<false>(results);
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::custom_recognize(const cv::Mat& image,
                                                               const MAA_VISION_NS::CustomRecognizerParam& param,
                                                               const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!inst_) {
        LogError << "Instance not binded";
        return std::nullopt;
    }

    auto recognizer = inst_->custom_recognizer(param.name);
    if (!recognizer) {
        LogError << "Custom recognizer not found:" << param.name;
        return std::nullopt;
    }
    recognizer->set_image(image);
    recognizer->set_param(param);
    recognizer->set_name(name);

    auto results = recognizer->analyze();
    if (results.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = results.front().box;
    auto detail = json::serialize<false>(results);
    return Result { .box = box, .detail = std::move(detail) };
}

MAA_TASK_NS_END
