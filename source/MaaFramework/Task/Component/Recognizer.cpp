#include "Recognizer.h"

#include "CustomRecognition.h"
#include "Global/OptionMgr.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
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

    RecoResult result = run_recognition(pipeline_data.reco_type, pipeline_data.reco_param, pipeline_data.name);

    if (debug_mode()) {
        ImageEncodedBuffer png;
        cv::imencode(".png", image_, png);
        result.raw = std::move(png);
    }

    if (pipeline_data.inverse) {
        LogDebug << "pipeline_data.inverse is true, reverse the result" << VAR(pipeline_data.name) << VAR(result.box);
        result.box = result.box ? std::nullopt : std::make_optional<cv::Rect>();
    }

    LogInfo << "reco" << VAR(result);
    auto& rt_cache = tasker_->runtime_cache();
    rt_cache.set_reco_detail(result.reco_id, result);

    save_draws(pipeline_data.name, result);

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
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "DirectHit",
        .box = cv::Rect {},
    };
}

RecoResult Recognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = context_.get_images(param.template_);
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

RecoResult Recognizer::feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    cv::Rect roi = get_roi(param.roi_target);

    auto templs = context_.get_images(param.template_);
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

RecoResult Recognizer::color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name)
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

    return RecoResult { .reco_id = reco_id_,
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

    return RecoResult { .reco_id = reco_id_,
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

    if (!resource()) {
        LogError << "resource is null";
        return {};
    }
    cv::Rect roi = get_roi(param.roi_target);

    auto session = resource()->custom_recognition(param.name);
    CustomRecognition analyzer(image_, roi, param, session, context_, name);

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

RecoResult Recognizer::and_(const std::shared_ptr<MAA_RES_NS::Recognition::AndParam>& param, const std::string& name)
{
    if (!param) {
        LogError << "AndParam is null";
        return {};
    }

    LogDebug << "And recognition" << VAR(name) << VAR(param->all_of.size()) << VAR(param->box_index);

    std::vector<RecoResult> sub_results;
    std::unordered_map<std::string, cv::Rect> sub_name_boxes;
    std::vector<ImageEncodedBuffer> all_draws;
    json::array sub_details;

    for (size_t i = 0; i < param->all_of.size(); ++i) {
        const auto& sub_reco = param->all_of[i];

        for (const auto& [sub_name, box] : sub_name_boxes) {
            auto& rt_cache = tasker_->runtime_cache();
            rt_cache.set_latest_node(sub_name, MaaInvalidId);

            RecoResult temp_result {
                .reco_id = reco_id_,
                .name = sub_name,
                .algorithm = "And.SubRef",
                .box = box,
            };
            rt_cache.set_reco_detail(reco_id_, temp_result);

            NodeDetail temp_node { .node_id = MaaInvalidId, .name = sub_name, .reco_id = reco_id_ };
            rt_cache.set_node_detail(MaaInvalidId, temp_node);
        }

        RecoResult sub_result = run_recognition(sub_reco.type, sub_reco.param, name + ".all_of[" + std::to_string(i) + "]");

        for (auto& draw : sub_result.draws) {
            all_draws.emplace_back(std::move(draw));
        }

        if (!sub_result.box) {
            LogDebug << "And: sub recognition failed at index" << VAR(i) << VAR(sub_reco.sub_name);
            return RecoResult {
                .reco_id = reco_id_,
                .name = name,
                .algorithm = "And",
                .box = std::nullopt,
                .detail = json::object { { "failed_at", i }, { "sub_results", std::move(sub_details) } },
                .draws = std::move(all_draws),
            };
        }

        if (!sub_reco.sub_name.empty()) {
            sub_name_boxes[sub_reco.sub_name] = *sub_result.box;
        }

        sub_details.emplace_back(json::object {
            { "sub_name", sub_reco.sub_name },
            { "algorithm", sub_result.algorithm },
            { "box", json::array { sub_result.box->x, sub_result.box->y, sub_result.box->width, sub_result.box->height } },
            { "detail", sub_result.detail },
        });

        sub_results.emplace_back(std::move(sub_result));
    }

    std::optional<cv::Rect> final_box = sub_results[param->box_index].box;

    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "And",
        .box = std::move(final_box),
        .detail = json::object { { "sub_results", std::move(sub_details) }, { "box_index", param->box_index } },
        .draws = std::move(all_draws),
    };
}

RecoResult Recognizer::or_(const std::shared_ptr<MAA_RES_NS::Recognition::OrParam>& param, const std::string& name)
{
    if (!param) {
        LogError << "OrParam is null";
        return {};
    }

    LogDebug << "Or recognition" << VAR(name) << VAR(param->any_of.size());

    std::vector<ImageEncodedBuffer> all_draws;
    json::array sub_details;

    for (size_t i = 0; i < param->any_of.size(); ++i) {
        const auto& sub_reco = param->any_of[i];

        RecoResult sub_result = run_recognition(sub_reco.type, sub_reco.param, name + ".any_of[" + std::to_string(i) + "]");

        for (auto& draw : sub_result.draws) {
            all_draws.emplace_back(std::move(draw));
        }

        if (sub_result.box) {
            LogDebug << "Or: sub recognition succeeded at index" << VAR(i) << VAR(sub_reco.sub_name);
            sub_details.emplace_back(json::object {
                { "sub_name", sub_reco.sub_name },
                { "algorithm", sub_result.algorithm },
                { "box", json::array { sub_result.box->x, sub_result.box->y, sub_result.box->width, sub_result.box->height } },
                { "detail", sub_result.detail },
            });

            return RecoResult {
                .reco_id = reco_id_,
                .name = name,
                .algorithm = "Or",
                .box = std::move(sub_result.box),
                .detail = json::object { { "hit_index", i }, { "sub_results", std::move(sub_details) } },
                .draws = std::move(all_draws),
            };
        }

        sub_details.emplace_back(json::object {
            { "sub_name", sub_reco.sub_name },
            { "algorithm", sub_result.algorithm },
            { "box", nullptr },
            { "detail", sub_result.detail },
        });
    }

    LogDebug << "Or: all sub recognitions failed";
    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "Or",
        .box = std::nullopt,
        .detail = json::object { { "sub_results", std::move(sub_details) } },
        .draws = std::move(all_draws),
    };
}

RecoResult
    Recognizer::run_recognition(MAA_RES_NS::Recognition::Type type, const MAA_RES_NS::Recognition::Param& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    switch (type) {
    case Type::DirectHit:
        return direct_hit(name);

    case Type::TemplateMatch:
        return template_match(std::get<TemplateMatcherParam>(param), name);

    case Type::FeatureMatch:
        return feature_match(std::get<FeatureMatcherParam>(param), name);

    case Type::ColorMatch:
        return color_match(std::get<ColorMatcherParam>(param), name);

    case Type::OCR:
        return ocr(std::get<OCRerParam>(param), name);

    case Type::NeuralNetworkClassify:
        return nn_classify(std::get<NeuralNetworkClassifierParam>(param), name);

    case Type::NeuralNetworkDetect:
        return nn_detect(std::get<NeuralNetworkDetectorParam>(param), name);

    case Type::And:
        return and_(std::get<std::shared_ptr<AndParam>>(param), name);

    case Type::Or:
        return or_(std::get<std::shared_ptr<OrParam>>(param), name);

    case Type::Custom:
        return custom_recognize(std::get<CustomRecognitionParam>(param), name);

    default:
        LogError << "Unknown recognition type" << VAR(static_cast<int>(type));
        return {};
    }
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
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    if (!option.save_draw()) {
        return;
    }

    auto dir = option.log_dir() / "vision";

    std::filesystem::create_directories(dir);

    for (const auto& draw : result.draws) {
        std::string filename = std::format("{}_{}_{}.jpg", node_name, result.reco_id, format_now_for_filename());
        auto filepath = dir / path(filename);

        std::ofstream of(filepath, std::ios::out | std::ios::binary);
        of.write(reinterpret_cast<const char*>(draw.data()), draw.size());
        LogDebug << "save draw to" << filepath;
    }
}

bool Recognizer::debug_mode() const
{
    return MAA_GLOBAL_NS::OptionMgr::get_instance().debug_mode();
}

MAA_RES_NS::ResourceMgr* Recognizer::resource()
{
    return tasker_ ? tasker_->resource() : nullptr;
}

MAA_TASK_NS_END
