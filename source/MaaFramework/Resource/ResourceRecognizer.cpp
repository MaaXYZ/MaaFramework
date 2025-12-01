#include "ResourceRecognizer.h"

#include "Global/OptionMgr.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "ResourceMgr.h"
#include "Vision/ColorMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/NeuralNetworkClassifier.h"
#include "Vision/NeuralNetworkDetector.h"
#include "Vision/OCRer.h"
#include "Vision/TemplateMatcher.h"
#include "Vision/VisionUtils.hpp"

MAA_RES_NS_BEGIN

ResourceRecognizer::ResourceRecognizer(ResourceMgr* resource, const cv::Mat& image)
    : resource_(resource)
    , image_(image)
{
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

MAA_TASK_NS::RecoResult ResourceRecognizer::recognize(Recognition::Type type, const Recognition::Param& param)
{
    using namespace MAA_VISION_NS;

    MAA_TASK_NS::RecoResult result;

    switch (type) {
    case Recognition::Type::TemplateMatch:
        result = template_match(std::get<TemplateMatcherParam>(param));
        break;
    case Recognition::Type::FeatureMatch:
        result = feature_match(std::get<FeatureMatcherParam>(param));
        break;
    case Recognition::Type::ColorMatch:
        result = color_match(std::get<ColorMatcherParam>(param));
        break;
    case Recognition::Type::OCR:
        result = ocr(std::get<OCRerParam>(param));
        break;
    case Recognition::Type::NeuralNetworkClassify:
        result = nn_classify(std::get<NeuralNetworkClassifierParam>(param));
        break;
    case Recognition::Type::NeuralNetworkDetect:
        result = nn_detect(std::get<NeuralNetworkDetectorParam>(param));
        break;
    default:
        LogError << "Unsupported recognition type for Resource" << VAR(static_cast<int>(type));
        return {};
    }

    if (debug_mode()) {
        result.raw = image_;
    }

    return result;
}

MAA_TASK_NS::RecoResult ResourceRecognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param)
{
    using namespace MAA_VISION_NS;

    if (!resource_) {
        LogError << "Resource is null";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = get_images(param.template_);
    TemplateMatcher analyzer(image_, roi, param, templs, "TemplateMatch");

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    MAA_TASK_NS::RecoResult result { .reco_id = analyzer.uid(),
                                     .name = "TemplateMatch",
                                     .algorithm = "TemplateMatch",
                                     .box = std::move(box),
                                     .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                                     .draws = std::move(analyzer).draws() };

    save_draws("TemplateMatch", result);

    return result;
}

MAA_TASK_NS::RecoResult ResourceRecognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param)
{
    using namespace MAA_VISION_NS;

    if (!resource_) {
        LogError << "Resource is null";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = get_images(param.template_);
    FeatureMatcher analyzer(image_, roi, param, templs, "FeatureMatch");

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    MAA_TASK_NS::RecoResult result { .reco_id = analyzer.uid(),
                                     .name = "FeatureMatch",
                                     .algorithm = "FeatureMatch",
                                     .box = std::move(box),
                                     .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                                     .draws = std::move(analyzer).draws() };

    save_draws("FeatureMatch", result);

    return result;
}

MAA_TASK_NS::RecoResult ResourceRecognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    ColorMatcher analyzer(image_, roi, param, "ColorMatch");

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    MAA_TASK_NS::RecoResult result { .reco_id = analyzer.uid(),
                                     .name = "ColorMatch",
                                     .algorithm = "ColorMatch",
                                     .box = std::move(box),
                                     .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                                     .draws = std::move(analyzer).draws() };

    save_draws("ColorMatch", result);

    return result;
}

MAA_TASK_NS::RecoResult ResourceRecognizer::ocr(const MAA_VISION_NS::OCRerParam& param)
{
    using namespace MAA_VISION_NS;

    if (!resource_) {
        LogError << "Resource is null";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto det_session = resource_->ocr_res().deter(param.model);
    auto reco_session = resource_->ocr_res().recer(param.model);
    auto ocr_session = resource_->ocr_res().ocrer(param.model);

    OCRer analyzer(image_, roi, param, det_session, reco_session, ocr_session, "OCR");

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    MAA_TASK_NS::RecoResult result { .reco_id = analyzer.uid(),
                                     .name = "OCR",
                                     .algorithm = "OCR",
                                     .box = std::move(box),
                                     .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                                     .draws = std::move(analyzer).draws() };

    save_draws("OCR", result);

    return result;
}

MAA_TASK_NS::RecoResult ResourceRecognizer::nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param)
{
    using namespace MAA_VISION_NS;

    if (!resource_) {
        LogError << "Resource is null";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto& onnx_res = resource_->onnx_res();
    const auto& session = onnx_res.classifier(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkClassifier analyzer(image_, roi, param, session, mem_info, "NNClassify");

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    MAA_TASK_NS::RecoResult result { .reco_id = analyzer.uid(),
                                     .name = "NNClassify",
                                     .algorithm = "NeuralNetworkClassify",
                                     .box = std::move(box),
                                     .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                                     .draws = std::move(analyzer).draws() };

    save_draws("NNClassify", result);

    return result;
}

MAA_TASK_NS::RecoResult ResourceRecognizer::nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param)
{
    using namespace MAA_VISION_NS;

    if (!resource_) {
        LogError << "Resource is null";
        return {};
    }

    cv::Rect roi = get_roi(param.roi_target);

    auto& onnx_res = resource_->onnx_res();
    const auto& session = onnx_res.detector(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkDetector analyzer(image_, roi, param, session, mem_info, "NNDetect");

    std::optional<cv::Rect> box = std::nullopt;
    if (analyzer.best_result()) {
        box = analyzer.best_result()->box;
    }

    MAA_TASK_NS::RecoResult result { .reco_id = analyzer.uid(),
                                     .name = "NNDetect",
                                     .algorithm = "NeuralNetworkDetect",
                                     .box = std::move(box),
                                     .detail = gen_detail(analyzer.all_results(), analyzer.filtered_results(), analyzer.best_result()),
                                     .draws = std::move(analyzer).draws() };

    save_draws("NNDetect", result);

    return result;
}

cv::Rect ResourceRecognizer::get_roi(const MAA_VISION_NS::Target& roi)
{
    using namespace MAA_VISION_NS;

    cv::Rect raw {};
    switch (roi.type) {
    case Target::Type::Self:
        LogError << "ROI target not support self";
        return {};

    case Target::Type::PreTask:
        // PreTask ROI is not supported in Resource context
        // because there's no task runtime cache
        LogError << "PreTask ROI is not supported in Resource context";
        return {};

    case Target::Type::Region:
        raw = std::get<cv::Rect>(roi.param);
        break;

    default:
        LogError << "Unknown target" << VAR(static_cast<int>(roi.type));
        return {};
    }

    return cv::Rect { raw.x + roi.offset.x, raw.y + roi.offset.y, raw.width + roi.offset.width, raw.height + roi.offset.height };
}

std::vector<cv::Mat> ResourceRecognizer::get_images(const std::vector<std::string>& names)
{
    if (!resource_) {
        LogError << "Resource is null";
        return {};
    }

    std::vector<cv::Mat> results;
    for (const std::string& name : names) {
        auto imgs = resource_->template_res().get_image(name);
        results.insert(results.end(), std::make_move_iterator(imgs.begin()), std::make_move_iterator(imgs.end()));
    }
    return results;
}

void ResourceRecognizer::save_draws(const std::string& name, const MAA_TASK_NS::RecoResult& result) const
{
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    if (!option.save_draw()) {
        return;
    }

    auto dir = option.log_dir() / "vision";

    for (const auto& draw : result.draws) {
        std::string filename = std::format("{}_{}_{}.png", name, result.reco_id, format_now_for_filename());
        auto filepath = dir / path(filename);
        imwrite(filepath, draw);
        LogDebug << "save draw to" << filepath;
    }
}

bool ResourceRecognizer::debug_mode() const
{
    return MAA_GLOBAL_NS::OptionMgr::get_instance().debug_mode();
}

MAA_RES_NS_END
