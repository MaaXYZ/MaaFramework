#include "Recognizer.h"

#include <condition_variable>
#include <mutex>
#include <unordered_set>

#include <boost/asio/post.hpp>

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
    , sub_filtered_boxes_(std::make_shared<typename decltype(sub_filtered_boxes_)::element_type>())
    , sub_best_box_(std::make_shared<typename decltype(sub_best_box_)::element_type>())
{
}

Recognizer::Recognizer(const Recognizer& recognizer)
    : tasker_(recognizer.tasker_)
    , context_(recognizer.context_)
    , image_(recognizer.image_)
    // do not copy reco_id_
    , sub_filtered_boxes_(recognizer.sub_filtered_boxes_)
    , sub_best_box_(recognizer.sub_best_box_)
{
}

RecoResult Recognizer::recognize(MAA_RES_NS::Recognition::Type type, const MAA_RES_NS::Recognition::Param& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    RecoResult result;

    switch (type) {
    case Type::DirectHit:
        result = direct_hit(std::get<DirectHitParam>(param), name);
        break;

    case Type::TemplateMatch:
        result = template_match(std::get<TemplateMatcherParam>(param), name);
        break;

    case Type::FeatureMatch:
        result = feature_match(std::get<FeatureMatcherParam>(param), name);
        break;

    case Type::ColorMatch:
        result = color_match(std::get<ColorMatcherParam>(param), name);
        break;

    case Type::OCR:
        result = ocr(std::get<OCRerParam>(param), name);
        break;

    case Type::NeuralNetworkClassify:
        result = nn_classify(std::get<NeuralNetworkClassifierParam>(param), name);
        break;

    case Type::NeuralNetworkDetect:
        result = nn_detect(std::get<NeuralNetworkDetectorParam>(param), name);
        break;

    case Type::And:
        result = and_(std::get<std::shared_ptr<AndParam>>(param), name);
        break;

    case Type::Or:
        result = or_(std::get<std::shared_ptr<OrParam>>(param), name);
        break;

    case Type::Custom:
        result = custom_recognize(std::get<CustomRecognitionParam>(param), name);
        break;

    default:
        LogError << "Unknown recognition type" << VAR(static_cast<int>(type)) << VAR(name);
        break;
    }

    if (debug_mode() && !image_.empty()) {
        ImageEncodedBuffer png;
        cv::imencode(".png", image_, png);
        result.raw = std::move(png);
    }

    LogInfo << "reco" << VAR(result);
    auto& rt_cache = tasker_->runtime_cache();
    rt_cache.set_reco_detail(result.reco_id, result);

    save_draws(name, result);

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

template <typename Res>
std::vector<cv::Rect> get_boxes(const std::vector<Res>& results)
{
    std::vector<cv::Rect> boxes;
    for (const auto& res : results) {
        boxes.emplace_back(res.box);
    }
    return boxes;
}

RecoResult Recognizer::direct_hit(const MAA_VISION_NS::DirectHitParam& param, const std::string& name)
{
    LogDebug << name;

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    // DirectHit: 使用第一个 ROI 作为 box
    cv::Rect box = rois.empty() ? cv::Rect {} : rois.front();

    sub_filtered_boxes_->insert_or_assign(name, rois);
    sub_best_box_->insert_or_assign(name, box);

    return RecoResult {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "DirectHit",
        .box = box,
    };
}

RecoResult Recognizer::template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name)
{
    using namespace MAA_VISION_NS;

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto templs = context_.get_images(param.template_);
    TemplateMatcher analyzer(image_, rois, param, templs, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto templs = context_.get_images(param.template_);
    FeatureMatcher analyzer(image_, rois, param, templs, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    ColorMatcher analyzer(image_, rois, param, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (!resource()) {
        LogError << "Resource not bound or status is null" << VAR(resource());
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto det_session = resource()->ocr_res().deter(param.model);
    auto reco_session = resource()->ocr_res().recer(param.model);
    auto ocr_session = resource()->ocr_res().ocrer(param.model);

    OCRer analyzer(image_, rois, param, det_session, reco_session, ocr_session, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto& onnx_res = resource()->onnx_res();
    const auto& session = onnx_res.classifier(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkClassifier analyzer(image_, rois, param, session, mem_info, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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

    if (image_.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (!resource()) {
        LogError << "Resource not bound";
        return {};
    }

    std::vector<cv::Rect> rois = get_rois(param.roi_target);

    auto& onnx_res = resource()->onnx_res();
    const auto& session = onnx_res.detector(param.model);
    const auto& mem_info = onnx_res.memory_info();

    NeuralNetworkDetector analyzer(image_, rois, param, session, mem_info, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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
    std::vector<cv::Rect> rois = get_rois(param.roi_target, true);

    auto session = resource()->custom_recognition(param.name);
    CustomRecognition analyzer(image_, rois.empty() ? cv::Rect {} : rois.front(), param, session, context_, name);

    sub_filtered_boxes_->insert_or_assign(name, get_boxes(analyzer.filtered_results()));
    sub_best_box_->insert_or_assign(name, analyzer.best_result() ? analyzer.best_result()->box : cv::Rect {});

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
    using namespace MAA_RES_NS::Recognition;

    if (!param) {
        LogError << "AndParam is null";
        return {};
    }

    LogDebug << "And recognition" << VAR(name) << VAR(param->all_of.size()) << VAR(param->box_index);

    // 多个子识别器时使用并行版本（但包含 Custom 识别时回退串行，Agent IPC 不是线程安全的）
    if (param->all_of.size() > 1) {
        bool has_custom = std::ranges::any_of(param->all_of, [this](const SubRecognition& sub) {
            if (auto* node_name = std::get_if<std::string>(&sub)) {
                auto node_opt = context_.get_pipeline_data(*node_name);
                return node_opt && node_opt->reco_type == Type::Custom;
            }
            else {
                return std::get<InlineSubRecognition>(sub).type == Type::Custom;
            }
        });
        if (!has_custom && !has_sub_dependencies(param->all_of)) {
            return and_parallel(param, name);
        }
    }

    std::vector<RecoResult> sub_results;
    bool all_hit = true;

    for (const auto& sub_reco : param->all_of) {
        Recognizer sub_recognizer(*this);
        RecoResult res;

        if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
            // Resolve node name to get recognition params
            auto node_opt = context_.get_pipeline_data(*node_name);
            if (!node_opt) {
                LogError << "And: failed to get pipeline data for node" << VAR(*node_name);
                all_hit = false;
                break;
            }
            LogDebug << "And: run node reference" << VAR(*node_name);
            res = sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
        }
        else {
            const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
            LogDebug << "And: run inline sub recognition" << VAR(inline_sub.type) << VAR(inline_sub.sub_name);
            res = sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
        }

        all_hit &= res.box.has_value();
        sub_results.emplace_back(std::move(res));

        if (!all_hit) {
            LogDebug << "And: sub recognition failed";
            break;
        }
    }

    std::vector<ImageEncodedBuffer> all_draws;
    for (auto& sub : sub_results) {
        all_draws.insert(all_draws.end(), std::make_move_iterator(sub.draws.begin()), std::make_move_iterator(sub.draws.end()));
    }

    RecoResult result {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "And",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!all_hit) {
        LogDebug << "And recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (static_cast<int>(sub_results.size()) <= param->box_index) {
        LogError << "all hit, but box_index is out of range" << VAR(name) << VAR(sub_results.size()) << VAR(param->box_index);
        result.box = std::nullopt;
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    result.box = std::move(sub_results[param->box_index].box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

RecoResult Recognizer::or_(const std::shared_ptr<MAA_RES_NS::Recognition::OrParam>& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;

    if (!param) {
        LogError << "OrParam is null";
        return {};
    }

    LogDebug << "Or recognition" << VAR(name) << VAR(param->any_of.size());

    // 多个子识别器时使用并行版本（但包含 Custom 识别时回退串行，Agent IPC 不是线程安全的）
    if (param->any_of.size() > 1) {
        bool has_custom = std::ranges::any_of(param->any_of, [this](const SubRecognition& sub) {
            if (auto* node_name = std::get_if<std::string>(&sub)) {
                auto node_opt = context_.get_pipeline_data(*node_name);
                return node_opt && node_opt->reco_type == Type::Custom;
            }
            else {
                return std::get<InlineSubRecognition>(sub).type == Type::Custom;
            }
        });
        if (!has_custom && !has_sub_dependencies(param->any_of)) {
            return or_parallel(param, name);
        }
    }

    std::vector<RecoResult> sub_results;

    bool has_hit = false;

    for (const auto& sub_reco : param->any_of) {
        Recognizer sub_recognizer(*this);
        RecoResult res;

        if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
            // Resolve node name to get recognition params
            auto node_opt = context_.get_pipeline_data(*node_name);
            if (!node_opt) {
                LogError << "Or: failed to get pipeline data for node" << VAR(*node_name);
                continue;
            }
            LogDebug << "Or: run node reference" << VAR(*node_name);
            res = sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
        }
        else {
            const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
            LogDebug << "Or: run inline sub recognition" << VAR(inline_sub.type) << VAR(inline_sub.sub_name);
            res = sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
        }

        has_hit = res.box.has_value();
        sub_results.emplace_back(std::move(res));

        if (has_hit) {
            LogDebug << "Or: sub recognition succeeded";
            break;
        }
    }
    std::vector<ImageEncodedBuffer> all_draws;
    for (auto& sub : sub_results) {
        all_draws.insert(all_draws.end(), std::make_move_iterator(sub.draws.begin()), std::make_move_iterator(sub.draws.end()));
    }

    RecoResult result {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "Or",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!has_hit) {
        LogDebug << "Or recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (sub_results.empty()) {
        LogError << "has hit, but no sub results" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return {};
    }

    result.box = std::move(sub_results.back().box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    // 按理说这里要从 sub 取的，但是太麻烦而且是 corner case，先不管了，后面有需要再加
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

RecoResult Recognizer::and_parallel(const std::shared_ptr<MAA_RES_NS::Recognition::AndParam>& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;

    LogDebug << "And parallel recognition" << VAR(name) << VAR(param->all_of.size());

    auto cancelled = std::make_shared<std::atomic<bool>>(false);

    struct ParallelState
    {
        std::mutex mutex;
        std::vector<std::pair<size_t, RecoResult>> results;
        std::atomic<size_t> completed_count { 0 };
        std::atomic<bool> has_failure { false };
        std::condition_variable cv;
    };

    auto state = std::make_shared<ParallelState>();
    const size_t total = param->all_of.size();

    auto& pool = tasker_->reco_thread_pool();

    for (size_t i = 0; i < param->all_of.size(); ++i) {
        const auto& sub_reco = param->all_of[i];

        boost::asio::post(pool, [this, &sub_reco, i, cancelled, state]() {
            if (cancelled->load(std::memory_order_acquire) || state->has_failure.load(std::memory_order_acquire)) {
                state->completed_count.fetch_add(1, std::memory_order_release);
                state->cv.notify_one();
                return;
            }

            Recognizer sub_recognizer(*this);
            RecoResult res;

            if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
                auto node_opt = context_.get_pipeline_data(*node_name);
                if (!node_opt) {
                    LogError << "And parallel: failed to get pipeline data for node" << VAR(*node_name);
                    state->has_failure.store(true, std::memory_order_release);
                    cancelled->store(true, std::memory_order_release);
                    state->completed_count.fetch_add(1, std::memory_order_release);
                    state->cv.notify_one();
                    return;
                }
                res = sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
            }
            else {
                const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
                res = sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
            }

            if (!res.box.has_value()) {
                state->has_failure.store(true, std::memory_order_release);
                cancelled->store(true, std::memory_order_release);
            }

            {
                std::lock_guard lock(state->mutex);
                state->results.emplace_back(i, std::move(res));
            }

            state->completed_count.fetch_add(1, std::memory_order_release);
            state->cv.notify_one();
        });
    }

    {
        std::unique_lock lock(state->mutex);
        state->cv.wait(lock, [&state, total]() {
            return state->completed_count.load(std::memory_order_acquire) >= total;
        });
    }

    // 按原始顺序排序结果
    std::sort(state->results.begin(), state->results.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::vector<RecoResult> sub_results;
    for (auto& [idx, res] : state->results) {
        sub_results.emplace_back(std::move(res));
    }

    bool all_hit = !state->has_failure.load(std::memory_order_acquire);

    std::vector<ImageEncodedBuffer> all_draws;
    for (auto& sub : sub_results) {
        all_draws.insert(all_draws.end(), std::make_move_iterator(sub.draws.begin()), std::make_move_iterator(sub.draws.end()));
    }

    RecoResult result {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "And",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!all_hit) {
        LogDebug << "And parallel recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    if (static_cast<int>(sub_results.size()) <= param->box_index) {
        LogError << "all hit, but box_index is out of range" << VAR(name) << VAR(sub_results.size()) << VAR(param->box_index);
        result.box = std::nullopt;
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    result.box = std::move(sub_results[param->box_index].box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

RecoResult Recognizer::or_parallel(const std::shared_ptr<MAA_RES_NS::Recognition::OrParam>& param, const std::string& name)
{
    using namespace MAA_RES_NS::Recognition;

    LogDebug << "Or parallel recognition" << VAR(name) << VAR(param->any_of.size());

    auto cancelled = std::make_shared<std::atomic<bool>>(false);

    struct ParallelState
    {
        std::mutex mutex;
        std::vector<std::pair<size_t, RecoResult>> results;
        std::atomic<size_t> completed_count { 0 };
        size_t first_hit_index = SIZE_MAX;
        std::condition_variable cv;
    };

    auto state = std::make_shared<ParallelState>();
    const size_t total = param->any_of.size();

    auto& pool = tasker_->reco_thread_pool();

    for (size_t i = 0; i < param->any_of.size(); ++i) {
        const auto& sub_reco = param->any_of[i];

        boost::asio::post(pool, [this, &sub_reco, i, cancelled, state]() {
            if (cancelled->load(std::memory_order_acquire)) {
                state->completed_count.fetch_add(1, std::memory_order_release);
                state->cv.notify_one();
                return;
            }

            Recognizer sub_recognizer(*this);
            RecoResult res;

            if (auto* node_name = std::get_if<std::string>(&sub_reco)) {
                auto node_opt = context_.get_pipeline_data(*node_name);
                if (!node_opt) {
                    LogError << "Or parallel: failed to get pipeline data for node" << VAR(*node_name);
                    state->completed_count.fetch_add(1, std::memory_order_release);
                    state->cv.notify_one();
                    return;
                }
                res = sub_recognizer.recognize(node_opt->reco_type, node_opt->reco_param, *node_name);
            }
            else {
                const auto& inline_sub = std::get<InlineSubRecognition>(sub_reco);
                res = sub_recognizer.recognize(inline_sub.type, inline_sub.param, inline_sub.sub_name);
            }

            {
                std::lock_guard lock(state->mutex);
                state->results.emplace_back(i, std::move(res));
                if (state->results.back().second.box.has_value() && i < state->first_hit_index) {
                    state->first_hit_index = i;
                    cancelled->store(true, std::memory_order_release);
                }
            }

            state->completed_count.fetch_add(1, std::memory_order_release);
            state->cv.notify_one();
        });
    }

    {
        std::unique_lock lock(state->mutex);
        state->cv.wait(lock, [&state, total]() {
            return state->completed_count.load(std::memory_order_acquire) >= total;
        });
    }

    // 按原始顺序排序结果
    std::sort(state->results.begin(), state->results.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::vector<RecoResult> sub_results;
    for (auto& [idx, res] : state->results) {
        sub_results.emplace_back(std::move(res));
    }

    bool has_hit = state->first_hit_index != SIZE_MAX;

    std::vector<ImageEncodedBuffer> all_draws;
    for (auto& sub : sub_results) {
        all_draws.insert(all_draws.end(), std::make_move_iterator(sub.draws.begin()), std::make_move_iterator(sub.draws.end()));
    }

    RecoResult result {
        .reco_id = reco_id_,
        .name = name,
        .algorithm = "Or",
        .detail = sub_results,
        .draws = std::move(all_draws),
    };

    if (!has_hit) {
        LogDebug << "Or parallel recognition failed" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    // 找到第一个命中的结果
    RecoResult* hit_result = nullptr;
    for (auto& sub : sub_results) {
        if (sub.box.has_value()) {
            hit_result = &sub;
            break;
        }
    }

    if (!hit_result) {
        LogError << "has hit, but no hit result found" << VAR(name);
        sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> {});
        sub_best_box_->insert_or_assign(name, cv::Rect {});
        return result;
    }

    result.box = std::move(hit_result->box);

    cv::Rect final_box = result.box.value_or(cv::Rect {});
    sub_filtered_boxes_->insert_or_assign(name, std::vector<cv::Rect> { final_box });
    sub_best_box_->insert_or_assign(name, final_box);

    return result;
}

bool Recognizer::has_sub_dependencies(const std::vector<MAA_RES_NS::Recognition::SubRecognition>& subs)
{
    using namespace MAA_RES_NS::Recognition;

    // 边收集 sub_name 边检查依赖（依赖总是"后面引用前面"的模式）
    std::unordered_set<std::string> sub_names;

    auto check_roi_dependency = [&sub_names](const MAA_VISION_NS::Target& roi) -> bool {
        if (roi.type == MAA_VISION_NS::TargetType::PreTask) {
            const auto& ref_name = std::get<std::string>(roi.param);
            return sub_names.contains(ref_name);
        }
        return false;
    };

    for (const auto& sub : subs) {
        if (auto* inline_sub = std::get_if<InlineSubRecognition>(&sub)) {
            // 先检查当前子识别器的 roi 是否引用了已收集的 sub_name
            bool has_dep = std::visit(
                [&check_roi_dependency](const auto& param) -> bool {
                    using T = std::decay_t<decltype(param)>;
                    if constexpr (std::is_base_of_v<MAA_VISION_NS::RoiTargetParamBase, T>) {
                        return check_roi_dependency(param.roi_target);
                    }
                    else if constexpr (std::is_same_v<T, std::shared_ptr<AndParam>> ||
                                       std::is_same_v<T, std::shared_ptr<OrParam>>) {
                        // And/Or 嵌套的情况暂不处理，回退串行
                        return true;
                    }
                    else {
                        return false;
                    }
                },
                inline_sub->param);

            if (has_dep) {
                LogDebug << "Found sub-recognition dependency, fallback to serial" << VAR(inline_sub->sub_name);
                return true;
            }

            // 然后将当前子识别器的 sub_name 加入集合
            if (!inline_sub->sub_name.empty()) {
                sub_names.insert(inline_sub->sub_name);
            }
        }
    }

    return false;
}

std::vector<cv::Rect> Recognizer::get_rois(const MAA_VISION_NS::Target& roi, bool use_best)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    using namespace MAA_VISION_NS;

    std::vector<cv::Rect> results;

    switch (roi.type) {
    case Target::Type::Self:
        LogError << "ROI target not support self";
        return {};

    case Target::Type::PreTask:
        results = get_rois_from_pretask(std::get<std::string>(roi.param), use_best);
        break;

    case Target::Type::Region:
        results = { std::get<cv::Rect>(roi.param) };
        break;

    default:
        LogError << "Unknown target" << VAR(static_cast<int>(roi.type));
        return {};
    }

    for (cv::Rect& res : results) {
        res.x += roi.offset.x;
        res.y += roi.offset.y;
        res.width += roi.offset.width;
        res.height += roi.offset.height;
    }
    return results;
}

std::vector<cv::Rect> Recognizer::get_rois_from_pretask(const std::string& name, bool use_best)
{
    if (use_best) {
        if (auto it = sub_best_box_->find(name); it != sub_best_box_->end()) {
            LogDebug << "pre task from sub_best_box_" << VAR(name) << VAR(it->second);
            return { it->second };
        }
    }
    else {
        if (auto it = sub_filtered_boxes_->find(name); it != sub_filtered_boxes_->end()) {
            LogDebug << "pre task from sub_filtered_boxes_" << VAR(name) << VAR(it->second.size());
            return it->second;
        }
    }

    // 回退到 runtime_cache
    auto& cache = tasker_->runtime_cache();
    MaaNodeId node_id = cache.get_latest_node(name).value_or(MaaInvalidId);
    NodeDetail node_detail = cache.get_node_detail(node_id).value_or(NodeDetail {});
    RecoResult reco_result = cache.get_reco_result(node_detail.reco_id).value_or(RecoResult {});
    cv::Rect raw = reco_result.box.value_or(cv::Rect {});
    LogDebug << "pre task from cache" << VAR(name) << VAR(raw);
    return { raw };
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
