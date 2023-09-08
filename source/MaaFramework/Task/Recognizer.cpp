#include "Recognizer.h"

#include "Instance/InstanceStatus.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.h"
#include "Vision/Classifier.h"
#include "Vision/CustomRecognizer.h"
#include "Vision/Detector.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
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
    cv::Rect cache {};
    if (task_data.cache) {
        cache = status()->get_pipeline_rec_box(task_data.name);
    }

    std::optional<Result> result;
    switch (task_data.rec_type) {
    case Type::DirectHit:
        result = direct_hit();
        break;

    case Type::TemplateMatch:
        result = template_match(image, std::get<TemplMatchingParam>(task_data.rec_param), cache, task_data.name);
        break;

    case Type::OCR:
        result = ocr(image, std::get<OcrParam>(task_data.rec_param), cache, task_data.name);
        break;

    case Type::Classify:
        result = classify(image, std::get<ClassifierParam>(task_data.rec_param), task_data.name);
        break;

    case Type::Detect:
        result = detect(image, std::get<DetectorParam>(task_data.rec_param), task_data.name);
        break;

    case Type::Custom:
        result = custom_recognize(image, std::get<CustomParam>(task_data.rec_param), cache, task_data.name);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(task_data.rec_type)) << VAR(task_data.name);
        return std::nullopt;
    }

    if (result) {
        status()->set_pipeline_rec_box(task_data.name, result->box);
        status()->set_pipeline_rec_detail(task_data.name, result->detail);
    }

    if (task_data.inverse) {
        LogDebug << "task_data.inverse is true, reverse the result" << VAR(task_data.name) << VAR(result.has_value());
        return result ? std::nullopt : std::make_optional(Result { .box = cv::Rect() });
    }
    return result;
}

std::optional<Recognizer::Result> Recognizer::direct_hit()
{
    return Result { .box = cv::Rect(), .detail = json::array() };
}

std::optional<Recognizer::Result> Recognizer::template_match(const cv::Mat& image,
                                                             const MAA_VISION_NS::TemplMatchingParam& param,
                                                             const cv::Rect& cache, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    Matcher matcher(inst_, image);
    matcher.set_name(name);
    matcher.set_param(param);
    matcher.set_cache(cache);

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

    auto ret = matcher.analyze();
    if (ret.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = ret.front().box;
    json::array detail;
    for (const auto& res : ret) {
        detail.emplace_back(res.to_json());
    }
    return Result { .box = box, .detail = detail.to_string() };
}

std::optional<Recognizer::Result> Recognizer::ocr(const cv::Mat& image, const MAA_VISION_NS::OcrParam& param,
                                                  const cv::Rect& cache, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    OCRer ocrer(inst_, image);
    ocrer.set_name(name);
    ocrer.set_param(param);
    ocrer.set_cache(cache);

    auto det_session = resource()->ocr_res().deter(param.model);
    auto rec_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);
    ocrer.set_session(std::move(det_session), std::move(rec_session), std::move(ocr_session));

    auto ret = ocrer.analyze();
    if (ret.empty()) {
        return std::nullopt;
    }

    // TODO: sort by required regex.
    // sort_by_required_(res, param.text);

    const cv::Rect& box = ret.front().box;
    json::array detail;
    for (const auto& res : ret) {
        detail.emplace_back(res.to_json());
    }
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::classify(const cv::Mat& image,
                                                       const MAA_VISION_NS::ClassifierParam& param,
                                                       const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    Classifier classifier(inst_, image);
    classifier.set_name(name);
    classifier.set_param(param);

    auto session = resource()->onnx_res().classifier(param.model);
    classifier.set_session(std::move(session));

    auto ret = classifier.analyze();
    if (ret.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = ret.front().box;
    json::array detail;
    for (const auto& res : ret) {
        detail.emplace_back(res.to_json());
    }
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::detect(const cv::Mat& image, const MAA_VISION_NS::DetectorParam& param,
                                                     const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }

    Detector detector(inst_, image);
    detector.set_name(name);
    detector.set_param(param);

    auto session = resource()->onnx_res().detector(param.model);
    detector.set_session(std::move(session));

    auto ret = detector.analyze();
    if (ret.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = ret.front().box;
    json::array detail;
    for (const auto& res : ret) {
        detail.emplace_back(res.to_json());
    }
    return Result { .box = box, .detail = std::move(detail) };
}

std::optional<Recognizer::Result> Recognizer::custom_recognize(const cv::Mat& image,
                                                               const MAA_VISION_NS::CustomParam& param,
                                                               const cv::Rect& cache, const std::string& name)
{
    using namespace MAA_VISION_NS;

    std::ignore = cache;

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

    auto ret = recognizer->analyze();
    if (ret.empty()) {
        return std::nullopt;
    }

    const cv::Rect& box = ret.front().box;
    json::array detail;
    for (const auto& res : ret) {
        detail.emplace_back(res.to_json());
    }

    return Result { .box = box, .detail = std::move(detail) };
}

MAA_TASK_NS_END
