#pragma once

#include <atomic>
#include <meojson/json.hpp>
#include <unordered_set>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "Resource/PipelineTypes.h"
#include "Task/Context.h"
#include "Task/PipelineTask.h"
#include "Tasker/Tasker.h"
#include "Vision/OCRer.h"

MAA_TASK_NS_BEGIN

class Recognizer
{
public:
public:
    Recognizer(Tasker* tasker, Context& context, const cv::Mat& image, std::shared_ptr<MAA_VISION_NS::OCRCache> ocr_batch_cache = nullptr);
    Recognizer(const Recognizer& recognizer);

public:
    static MaaRecoId generate_reco_id() { return ++s_global_reco_id; }

    RecoResult recognize(MAA_RES_NS::Recognition::Type type, const MAA_RES_NS::Recognition::Param& param, const std::string& name);

    void prefetch_batch_ocr(const std::vector<BatchOCREntry>& entries);

    MaaRecoId get_id() const { return reco_id_; }

private:
    RecoResult direct_hit(const MAA_VISION_NS::DirectHitParam& param, const std::string& name);
    RecoResult template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name);
    RecoResult feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name);
    RecoResult color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name);
    RecoResult ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name);
    RecoResult nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name);
    RecoResult nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name);
    RecoResult and_(const std::shared_ptr<MAA_RES_NS::Recognition::AndParam>& param, const std::string& name);
    RecoResult or_(const std::shared_ptr<MAA_RES_NS::Recognition::OrParam>& param, const std::string& name);
    RecoResult custom_recognize(const MAA_VISION_NS::CustomRecognitionParam& param, const std::string& name);

    struct AndBranchResult
    {
        bool hit = false;
        std::vector<RecoResult> sub_results;
    };

    RecoResult run_sub_recognition(const MAA_RES_NS::Recognition::SubRecognition& sub_reco, const std::string& combinator_name);
    AndBranchResult match_and_branch(const std::vector<MAA_RES_NS::Recognition::SubRecognition>& all_of, size_t index);
    std::vector<cv::Rect> get_candidate_boxes_for_and_branch(
        const std::vector<MAA_RES_NS::Recognition::SubRecognition>& all_of,
        size_t index,
        const RecoResult& result) const;
    bool later_sub_recognitions_use_roi_target(
        const std::vector<MAA_RES_NS::Recognition::SubRecognition>& all_of,
        size_t index,
        const std::string& target_name) const;
    bool sub_recognition_uses_roi_target(
        const MAA_RES_NS::Recognition::SubRecognition& sub_reco,
        const std::string& target_name,
        std::unordered_set<std::string>& visited_nodes) const;
    bool recognition_param_uses_roi_target(
        const MAA_RES_NS::Recognition::Param& param,
        const std::string& target_name,
        std::unordered_set<std::string>& visited_nodes) const;

    template <typename Analyzer>
    RecoResult build_result(const std::string& name, const std::string& algorithm, Analyzer&& analyzer);

    std::vector<cv::Rect> get_rois(const MAA_VISION_NS::Target& roi, bool use_best = false);
    std::vector<cv::Rect> get_rois_from_pretask(const std::string& name, bool use_best);
    void save_draws(const std::string& node_name, const RecoResult& result) const;
    void register_sub_result_in_cache(const RecoResult& res);
    void commit_sub_node_detail(const RecoResult& res);
    void flush_pending_sub_results(size_t begin);

private:
    bool debug_mode() const;
    MAA_RES_NS::ResourceMgr* resource();

private:
    inline static std::atomic<MaaRecoId> s_global_reco_id = kRecoIdBase;

    Tasker* tasker_ = nullptr;
    Context& context_;
    cv::Mat image_;
    const MaaRecoId reco_id_ = generate_reco_id();

    std::shared_ptr<std::unordered_map<std::string, std::vector<cv::Rect>>> sub_filtered_boxes_;
    std::shared_ptr<std::unordered_map<std::string, cv::Rect>> sub_best_box_;
    std::shared_ptr<std::vector<RecoResult>> pending_sub_results_;
    bool cache_sub_results_ = true;

    std::shared_ptr<MAA_VISION_NS::OCRCache> ocr_batch_cache_;
};

MAA_TASK_NS_END
