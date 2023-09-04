#include "Recognizer.h"

#include "Instance/InstanceStatus.h"
#include "Utils/Logger.h"
#include "Vision/CustomRecognizer.h"
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

    Matcher matcher(inst_, image);
    matcher.set_param(param);
    matcher.set_cache(cache);
    matcher.set_name(name);

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

    OCRer ocrer(inst_, image);
    ocrer.set_param(param);
    ocrer.set_cache(cache);
    ocrer.set_name(name);

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
