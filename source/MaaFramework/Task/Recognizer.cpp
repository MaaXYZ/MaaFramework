#include "Recognizer.h"

#include "Global/GlobalOptionMgr.h"
#include "Global/UniqueResultBank.h"
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

    save_draws(task_data.name, result);
    UniqueResultBank::get_instance().add_reco_detail(result.uid, result);

    if (result.hit) {
        const auto& hit = *result.hit;
        show_hit_draw(image, hit, task_data.name, result.uid);
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

bool Recognizer::query_detail(
    MaaRecoId reco_id,
    bool& hit,
    cv::Rect& box,
    std::string& detail,
    std::vector<cv::Mat>& draws)
{
    std::any res_any = UniqueResultBank::get_instance().get_reco_detail(reco_id);

    if (!res_any.has_value()) {
        LogError << "reco_id has no value (not found)" << VAR(reco_id);
        return false;
    }

    auto res = std::any_cast<MAA_TASK_NS::Recognizer::Result>(res_any);

    hit = res.hit.has_value();
    if (hit) {
        box = *res.hit;
    }
    detail = res.detail.to_string();
    draws = res.draws;

    return true;
}

template <typename Res>
json::value gen_detail(
    const std::vector<Res>& all,
    const std::vector<Res>& filtered,
    const std::optional<Res>& best)
{
    std::ignore = best;

    return {
        { "all", json::array(all) },
        { "filtered", json::array(filtered) },
        { "best", best ? json::value(*best) : json::value(nullptr) },
    };
}

Recognizer::Result Recognizer::direct_hit(const std::string& name)
{
    LogTrace << name;
    return Result { .hit = cv::Rect {} };
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

    TemplateMatcher analyzer(image, param, templates, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
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

    FeatureMatcher analyzer(image, param, templates, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
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

    ColorMatcher analyzer(image, param, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
}

Recognizer::Result Recognizer::ocr(
    const cv::Mat& image,
    const MAA_VISION_NS::OCRerParam& param,
    const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (!resource()) {
        LogError << "Resource not binded or status is null" << VAR(resource());
        return {};
    }

    auto det_session = resource()->ocr_res().deter(param.model);
    auto rec_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer analyzer(image, param, det_session, rec_session, ocr_session, ocr_cache_, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
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

    NeuralNetworkClassifier analyzer(image, param, session, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
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

    NeuralNetworkDetector analyzer(image, param, session, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
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

    CustomRecognizer analyzer(image, param, *session, inst_, name);

    std::optional<Hit> hit = std::nullopt;
    if (analyzer.best_result()) {
        hit = analyzer.best_result()->box;
    }

    return Result {
        .uid = analyzer.uid(),
        .hit = std::move(hit),
        .detail =
            gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
        .draws = std::move(analyzer).draws()
    };
}

void Recognizer::save_draws(const std::string& task_name, const Result& result) const
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
    MaaRecoId uid) const
{
    if (!GlobalOptionMgr::get_instance().show_hit_draw()) {
        return;
    }

    const std::string kWinName = std::format("Hit: {} {}", task_name, uid);

    cv::Mat draw = image.clone();

    const cv::Scalar color(0, 255, 0);
    cv::rectangle(draw, res, color, 2);

    cv::imshow(kWinName, draw);
    cv::waitKey(0);
    cv::destroyWindow(kWinName);
}

MAA_TASK_NS_END