#include "Recognizer.h"

#include "Instance/InstanceStatus.h"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
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

Recognizer::Result Recognizer::recognize(const cv::Mat& image, const TaskData& task_data)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    if (!status()) {
        LogError << "Status is null";
        return {};
    }

    Result result;
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
        return {};
    }

    save_draws(result, task_data.name);

    if (result.hit) {
        status()->set_rec_box(task_data.name, result.hit->box);
        status()->set_rec_detail(task_data.name, result.hit->detail);

        show_hit_draw(image, *result.hit, task_data.name, result.uid);
    }

    if (task_data.inverse) {
        LogDebug << "task_data.inverse is true, reverse the result" << VAR(task_data.name)
                 << VAR(result.hit.has_value());

        if (result.hit) {
            result.hit = std::nullopt;
        }
        else {
            result.hit = Hit {};
        }
    }

    return result;
}

Recognizer::Result Recognizer::direct_hit(const std::string& name)
{
    LogTrace << name;
    return Result { .hit = Hit { .box = cv::Rect() } };
}

Recognizer::Result Recognizer::template_match(
    const cv::Mat& image,
    const MAA_VISION_NS::TemplateMatcherParam& param,
    const std::string& name)
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

    TemplateMatcher matcher(image, param, templates, name);

    auto results = std::move(matcher).filtered_results();
    size_t index = matcher.preferred_index();

    Result res { .uid = matcher.uid(), .draws = std::move(matcher).draws() };
    if (index >= results.size()) {
        return res;
    }

    res.hit = { .box = results[index].box, .detail = std::move(results) };
    return res;
}

Recognizer::Result Recognizer::feature_match(
    const cv::Mat& image,
    const MAA_VISION_NS::FeatureMatcherParam& param,
    const std::string& name)
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

    FeatureMatcher matcher(image, param, templates, name);

    auto results = std::move(matcher).filtered_results();
    size_t index = matcher.preferred_index();

    Result res { .uid = matcher.uid(), .draws = std::move(matcher).draws() };
    if (index >= results.size()) {
        return res;
    }

    res.hit = { .box = results[index].box, .detail = std::move(results) };
    return res;
}

Recognizer::Result Recognizer::color_match(
    const cv::Mat& image,
    const MAA_VISION_NS::ColorMatcherParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    ColorMatcher matcher(image, param, name);

    auto results = std::move(matcher).filtered_results();
    size_t index = matcher.preferred_index();
    auto draws = std::move(matcher).draws();

    Result res { .uid = matcher.uid(), .draws = std::move(matcher).draws() };
    if (index >= results.size()) {
        return res;
    }

    res.hit = { .box = results[index].box, .detail = std::move(results) };
    return res;
}

Recognizer::Result Recognizer::ocr(
    const cv::Mat& image,
    const MAA_VISION_NS::OCRerParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource() || !status()) {
        LogError << "Resource not binded or status is null" << VAR(resource()) << VAR(status());
        return {};
    }

    auto det_session = resource()->ocr_res().deter(param.model);
    auto rec_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer ocrer(image, param, det_session, rec_session, ocr_session, status(), name);

    auto results = std::move(ocrer).filtered_results();
    size_t index = ocrer.preferred_index();

    Result res { .uid = ocrer.uid(), .draws = std::move(ocrer).draws() };
    if (index >= results.size()) {
        return res;
    }

    res.hit = { .box = results[index].box, .detail = std::move(results) };
    return res;
}

Recognizer::Result Recognizer::nn_classify(
    const cv::Mat& image,
    const MAA_VISION_NS::NeuralNetworkClassifierParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    auto session = resource()->onnx_res().classifier(param.model);

    NeuralNetworkClassifier classifier(image, param, session, name);

    auto results = std::move(classifier).filtered_results();
    size_t index = classifier.preferred_index();

    Result res { .uid = classifier.uid(), .draws = std::move(classifier).draws() };
    if (index >= results.size()) {
        return res;
    }

    res.hit = { .box = results[index].box, .detail = std::move(results) };
    return res;
}

Recognizer::Result Recognizer::nn_detect(
    const cv::Mat& image,
    const MAA_VISION_NS::NeuralNetworkDetectorParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded";
        return {};
    }

    auto session = resource()->onnx_res().detector(param.model);

    NeuralNetworkDetector detector(image, param, session, name);

    auto results = std::move(detector).filtered_results();
    size_t index = detector.preferred_index();

    Result res { .uid = detector.uid(), .draws = std::move(detector).draws() };
    if (index >= results.size()) {
        return res;
    }

    res.hit = { .box = results[index].box, .detail = std::move(results) };
    return res;
}

Recognizer::Result Recognizer::custom_recognize(
    const cv::Mat& image,
    const MAA_VISION_NS::CustomRecognizerParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!inst_) {
        LogError << "Instance not binded";
        return {};
    }

    auto* session = inst_->custom_recognizer_session(param.name);
    if (!session) {
        LogError << "Custom recognizer not found:" << param.name;
        return {};
    }

    CustomRecognizer recognizer(image, param, *session, inst_, name);
    auto result = std::move(recognizer).result();
    bool ret = recognizer.ret();

    Result res { .uid = recognizer.uid() };
    if (!ret) {
        return res;
    }

    res.hit = Hit { .box = result.box, .detail = std::move(result) };
    return res;
}

void Recognizer::save_draws(const Result& result, const std::string& task_name) const
{
    const auto& option = GlobalOptionMgr::get_instance();

    if (!option.save_draw()) {
        return;
    }

    auto dir = option.log_dir() / "vision";

    for (const auto& draw : result.draws) {
        std::string filename =
            std::format("{}_{}_{}.png", task_name, result.uid, format_now_for_filename());
        auto filepath = dir / path(filename);
        imwrite(filepath, draw);
        LogDebug << "save draw to" << filepath;
    }
}

void Recognizer::show_hit_draw(
    const cv::Mat& image,
    const Hit& res,
    const std::string& task_name,
    int64_t uid) const
{
    if (!GlobalOptionMgr::get_instance().show_hit_draw()) {
        return;
    }

    const std::string kWinName = std::format("Hit: {} {}", task_name, uid);

    cv::Mat draw = image.clone();

    const cv::Scalar color(0, 255, 0);
    cv::rectangle(draw, res.box, color, 2);

    cv::imshow(kWinName, draw);
    cv::waitKey(0);
    cv::destroyWindow(kWinName);
}

MAA_TASK_NS_END
