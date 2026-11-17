#include "OCRCache.h"

#include <algorithm>
#include <iostream>

#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/OCRBatcher.h"
#include "Vision/OCRer.h"

namespace
{
using namespace MAA_VISION_NS;

class CountingOCRSession : public OCRSession
{
public:
    bool supports_cache() const override { return cache_supported; }

    std::optional<OCRDetRecResult> det_and_rec(const cv::Mat& image) override
    {
        inputs.emplace_back(image.clone());
        if (fail_next) {
            fail_next = false;
            return std::nullopt;
        }
        if (malformed_next) {
            malformed_next = false;
            return OCRDetRecResult { .texts = { "missing scores" } };
        }
        if (empty) {
            return OCRDetRecResult { };
        }
        return OCRDetRecResult {
            .boxes = { { 1, 2, 11, 2, 11, 8, 1, 8 }, { 13, 2, 23, 2, 23, 8, 13, 8 } },
            .texts = { " alpha ", "beta" },
            .scores = { 0.9f, 0.7f },
        };
    }

    std::optional<OCRRecResult> rec(const cv::Mat&) override
    {
        ++rec_runs;
        return OCRRecResult { "recognizer", 0.9f };
    }

    std::optional<std::vector<OCRRecResult>> batch_rec(const std::vector<cv::Mat>&) override
    {
        ++batch_rec_runs;
        return std::nullopt;
    }

    std::vector<cv::Mat> inputs;
    size_t rec_runs = 0;
    size_t batch_rec_runs = 0;
    bool fail_next = false;
    bool malformed_next = false;
    bool empty = false;
    bool cache_supported = true;
};

class CountingBatchOCRSession : public CountingOCRSession
{
public:
    std::optional<cv::Size> batch_det_shape(const cv::Size& image_size) const override
    {
        if (fixed_shape) {
            return fixed_shape;
        }
        return cv::Size((image_size.width + 31) / 32 * 32, (image_size.height + 31) / 32 * 32);
    }

    std::optional<OCRDetRecResult> det_and_rec(const cv::Mat& image) override
    {
        inputs.emplace_back(image.clone());
        return make_result(image);
    }

    std::optional<std::vector<OCRDetRecResult>> batch_det_and_rec(const std::vector<cv::Mat>& images) override
    {
        auto& batch = batch_inputs.emplace_back();
        for (const auto& image : images) {
            batch.emplace_back(image.clone());
        }
        if (fail_batch) {
            return std::nullopt;
        }

        std::vector<OCRDetRecResult> results;
        for (const auto& image : images) {
            results.emplace_back(make_result(image));
        }
        if (malformed_batch && !results.empty()) {
            results.front() = OCRDetRecResult { .texts = { "missing scores" } };
        }
        if (wrong_batch_size) {
            results.clear();
        }
        return results;
    }

    std::vector<std::vector<cv::Mat>> batch_inputs;
    std::optional<cv::Size> fixed_shape;
    bool fail_batch = false;
    bool malformed_batch = false;
    bool wrong_batch_size = false;

private:
    OCRDetRecResult make_result(const cv::Mat& image) const
    {
        if (empty) {
            return { };
        }
        return OCRDetRecResult {
            .boxes = { { 1, 2, 11, 2, 11, 8, 1, 8 }, { 13, 2, 23, 2, 23, 8, 13, 8 } },
            .texts = { " crop-" + std::to_string(image.at<cv::Vec3b>(0, 0)[0]) + " ", "beta" },
            .scores = { 0.9f, 0.7f },
        };
    }
};

bool check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "OCR cache regression: " << message << '\n';
    }
    return condition;
}

json::value details(const OCRer& ocr)
{
    return { { "all", ocr.all_results() }, { "filtered", ocr.filtered_results() }, { "best", ocr.best_result() } };
}

bool test_filters_and_rois(const cv::Mat& image)
{
    auto session = std::make_shared<CountingOCRSession>();
    OCRCache cache;
    const cv::Rect small(12, 20, 48, 32);
    const cv::Rect large(0, 0, 192, 128);
    const cv::Rect overlap(30, 20, 48, 32);
    OCRerParam param;
    param.expected = { L"missing" };
    OCRer miss(image, { small }, param, session, "first", std::nullopt, &cache);
    if (!check(!miss.best_result() && miss.all_results().size() == 2, "an expected miss must retain raw results")) {
        return false;
    }

    param.replace = { { L"alpha", L"renamed" } };
    param.expected = { L"^renamed$" };
    param.threshold = 0.8;
    OCRer hit(image, { small }, param, session, "second", std::nullopt, &cache);
    if (!check(
            hit.best_result() && hit.best_result()->text == L"renamed" && hit.all_results().front().text == L" alpha "
                && session->inputs.size() == 1,
            "nodes must share raw inference while applying their own replacements and expectations")) {
        return false;
    }

    param.expected.clear();
    param.threshold = 0;
    param.order_by = ResultOrderBy::Length;
    param.result_index = -1;
    OCRer indexed(image, { small }, param, session, "indexed", std::nullopt, &cache);
    param.threshold = 0.95;
    OCRer high_threshold(image, { small }, param, session, "threshold", std::nullopt, &cache);
    if (!check(
            indexed.best_result() && indexed.best_result()->text == L"beta" && !high_threshold.best_result() && session->inputs.size() == 1,
            "sorting, index and confidence filtering must be independent of the cached node")) {
        return false;
    }

    param = { };
    OCRer containing(image, { large }, param, session, "same-name", std::nullopt, &cache);
    OCRer intersecting(image, { overlap }, param, session, "same-name", std::nullopt, &cache);
    OCRer multi(image, { small, large, overlap }, param, session, "multiple", std::nullopt, &cache);
    OCRer solo(image, { small, large, overlap }, param, std::make_shared<CountingOCRSession>(), "solo");
    if (!check(
            session->inputs.size() == 3 && details(multi) == details(solo) && multi.all_results().size() == 6
                && session->batch_rec_runs == 0,
            "overlapping and same-name ROIs must preserve independent detection results without extra duplicates")) {
        return false;
    }
    const std::vector<cv::Rect> expected_rois { small, large, overlap };
    for (size_t i = 0; i < expected_rois.size(); ++i) {
        if (!check(
                session->inputs[i].size() == expected_rois[i].size()
                    && cv::norm(session->inputs[i], image(expected_rois[i]), cv::NORM_INF) == 0,
                "each detector input must be the unmodified original crop")) {
            return false;
        }
    }
    return true;
}

bool test_empty_failure_and_lifetime(const cv::Mat& image)
{
    const cv::Rect roi(0, 0, 48, 32);
    OCRerParam param;
    OCRCache cache;
    auto empty_session = std::make_shared<CountingOCRSession>();
    empty_session->empty = true;
    OCRer empty(image, { roi }, param, empty_session, "empty", std::nullopt, &cache);
    OCRer reused_empty(image, { roi }, param, empty_session, "empty-again", std::nullopt, &cache);
    if (!check(
            empty.all_results().empty() && reused_empty.all_results().empty() && empty_session->inputs.size() == 1,
            "successful empty results must be cached")) {
        return false;
    }

    auto session = std::make_shared<CountingOCRSession>();
    session->fail_next = true;
    OCRer failed(image, { roi }, param, session, "failed", std::nullopt, &cache);
    OCRer retried(image, { roi }, param, session, "retried", std::nullopt, &cache);
    OCRer reused(image, { roi }, param, session, "reused", std::nullopt, &cache);
    if (!check(
            failed.all_results().empty() && retried.best_result() && details(reused) == details(retried) && session->inputs.size() == 2,
            "inference failures must be retried, and different sessions must not share results")) {
        return false;
    }

    OCRCache next_frame;
    OCRer fresh(image, { roi }, param, session, "new-frame", std::nullopt, &next_frame);
    if (!check(fresh.best_result() && session->inputs.size() == 3, "a new screenshot must start with an empty cache")) {
        return false;
    }

    auto malformed_session = std::make_shared<CountingOCRSession>();
    malformed_session->malformed_next = true;
    OCRer malformed(image, { roi }, param, malformed_session, "malformed", std::nullopt, &cache);
    OCRer valid(image, { roi }, param, malformed_session, "valid", std::nullopt, &cache);
    if (!check(
            malformed.all_results().empty() && valid.best_result() && malformed_session->inputs.size() == 2,
            "malformed result arrays must not poison the cache")) {
        return false;
    }

    std::weak_ptr<OCRSession> previous_session;
    OCRCache lifetime_cache;
    {
        auto temporary = std::make_shared<CountingOCRSession>();
        previous_session = temporary;
        OCRer prediction(image, { roi }, param, temporary, "temporary", std::nullopt, &lifetime_cache);
    }
    if (!check(!previous_session.expired(), "cached sessions must stay alive to prevent address reuse")) {
        return false;
    }
    lifetime_cache = { };
    return check(previous_session.expired(), "releasing the cache must release the retained session");
}

bool test_incompatible_requests(const cv::Mat& image)
{
    const cv::Rect roi(0, 0, 48, 32);
    OCRerParam param;
    OCRCache cache;
    auto session = std::make_shared<CountingOCRSession>();
    OCRer original(image, { roi }, param, session, "original", std::nullopt, &cache);

    param.only_rec = true;
    for (int i = 0; i < 2; ++i) {
        OCRer rec(image, { roi }, param, session, "rec-only", std::nullopt, &cache);
        if (!check(rec.best_result() && rec.best_result()->text == L"recognizer", "only_rec must bypass det+rec cache")) {
            return false;
        }
    }
    param.only_rec = false;
    param.color_filter = "filter";
    ColorFilterConfig filter { .range = { { { 0, 0, 0 }, { 255, 255, 255 } } } };
    for (int i = 0; i < 2; ++i) {
        OCRer filtered(image, { roi }, param, session, "filtered", filter, &cache);
    }
    param.color_filter.clear();
    OCRer original_again(image, { roi }, param, session, "original-again", std::nullopt, &cache);
    if (!check(
            session->rec_runs == 2 && session->inputs.size() == 3 && details(original_again) == details(original)
                && cv::norm(session->inputs.back(), cv::Mat(roi.size(), CV_8UC3, cv::Scalar(255, 255, 255)), cv::NORM_INF) == 0,
            "filtered inputs must bypass the cache without overwriting unfiltered results")) {
        return false;
    }

    auto unsupported = std::make_shared<CountingOCRSession>();
    unsupported->cache_supported = false;
    for (int i = 0; i < 2; ++i) {
        OCRer uncached(image, { roi }, param, unsupported, "unsupported", std::nullopt, &cache);
    }
    return check(unsupported->inputs.size() == 2, "sessions without cache support must remain uncached");
}

std::vector<cv::Rect> batch_test_rois(size_t count)
{
    std::vector<cv::Rect> rois;
    for (size_t i = 0; i < count; ++i) {
        rois.emplace_back(static_cast<int>(i % 4) * 48, static_cast<int>(i / 4) * 48, 32, 32);
    }
    return rois;
}

bool test_batch_threshold(const cv::Mat& image)
{
    for (size_t count = 1; count < 4; ++count) {
        auto session = std::make_shared<CountingBatchOCRSession>();
        OCRCache cache;
        const auto rois = batch_test_rois(count);
        OCRBatcher(image, rois, session, cache);
        if (!check(
                session->batch_inputs.empty() && session->inputs.empty() && !cache.find(session, rois.front()),
                "one to three compatible ROIs must remain lazy without prefetching")) {
            return false;
        }
        OCRer first(image, { rois.front() }, { }, session, "below-batch-threshold", std::nullopt, &cache);
        if (!check(first.best_result() && session->inputs.size() == 1, "small groups must run only the requested ROI")) {
            return false;
        }
    }

    const auto rois = batch_test_rois(5);
    auto session = std::make_shared<CountingBatchOCRSession>();
    OCRCache cache;
    OCRBatcher(image, { rois[0], rois[1], rois[2], rois[0], rois[1], rois[2] }, session, cache);
    if (!check(session->batch_inputs.empty(), "duplicate ROIs must not count toward the four-ROI threshold")) {
        return false;
    }
    OCRBatcher(
        image,
        { rois[0], rois[1], rois[2], cv::Rect(0, 96, 48, 32), cv::Rect(64, 96, 48, 32), cv::Rect(128, 96, 48, 32) },
        session,
        cache);
    if (!check(session->batch_inputs.empty(), "different detector shapes must not combine to reach the threshold")) {
        return false;
    }

    OCRer cached(image, { rois[0] }, { }, session, "already-cached", std::nullopt, &cache);
    OCRBatcher(image, { rois[0], rois[1], rois[2], rois[3] }, session, cache);
    if (!check(
            session->batch_inputs.empty() && session->inputs.size() == 1 && !cache.find(session, rois[1]),
            "cached ROIs must not count toward the threshold or trigger eager solo work")) {
        return false;
    }
    OCRBatcher(image, rois, session, cache);
    return check(
        session->batch_inputs.size() == 1 && session->batch_inputs.front().size() == 4 && session->inputs.size() == 1
            && cache.find(session, rois[4]),
        "four uncached compatible ROIs must batch even when other candidates are already cached");
}

bool test_batch_prefetch_and_mapping(const cv::Mat& original_image)
{
    cv::Mat image = original_image.clone();
    const cv::Rect first(8, 10, 48, 32);
    const cv::Rect second(120, 90, 50, 30);
    const cv::Rect third(70, 20, 52, 29);
    const cv::Rect fourth(180, 40, 46, 31);
    const cv::Rect different_shape(0, 0, 96, 64);
    const std::vector<cv::Rect> compatible { first, second, third, fourth };
    for (size_t i = 0; i < compatible.size(); ++i) {
        image.at<cv::Vec3b>(compatible[i].y, compatible[i].x)[0] = static_cast<unsigned char>(17 + 12 * i);
    }
    auto session = std::make_shared<CountingBatchOCRSession>();
    OCRCache cache;

    OCRBatcher(image, { second, different_shape, first, second, fourth, third }, session, cache);
    if (!check(
            session->batch_inputs.size() == 1 && session->batch_inputs.front().size() == 4 && session->inputs.empty()
                && !cache.find(session, different_shape),
            "prefetch must batch four compatible crops, deduplicate identical ROIs and leave unmatched shapes lazy")) {
        return false;
    }
    const auto& batch = session->batch_inputs.front();
    for (const auto& input : batch) {
        const bool exact_crop = std::ranges::any_of(compatible, [&](const cv::Rect& roi) {
            return input.size() == roi.size() && cv::norm(input, image(roi), cv::NORM_INF) == 0;
        });
        if (!check(exact_crop, "tensor batch inputs must remain exact original crops")) {
            return false;
        }
    }

    OCRerParam param;
    OCRer from_batch(image, compatible, param, session, "batch", std::nullopt, &cache);
    OCRer standalone(image, compatible, param, std::make_shared<CountingBatchOCRSession>(), "standalone");
    if (!check(
            details(from_batch) == details(standalone) && session->inputs.empty(),
            "batch results must map back to their own ROI with correct coordinate offsets")) {
        return false;
    }

    param.expected = { L"^renamed$" };
    param.replace = { { L"crop-[0-9]+", L"renamed" } };
    param.threshold = 0.8;
    OCRer filtered(image, { first }, param, session, "filtered-batch", std::nullopt, &cache);
    if (!check(
            filtered.best_result() && filtered.best_result()->text == L"renamed" && filtered.all_results().front().text == L" crop-17 "
                && session->inputs.empty(),
            "prefetched results must retain raw text while each node applies its own filtering")) {
        return false;
    }

    OCRBatcher(image, { first, second, third, fourth, different_shape }, session, cache);
    OCRer unmatched(image, { different_shape }, { }, session, "unmatched-shape", std::nullopt, &cache);
    if (!check(
            unmatched.best_result() && session->inputs.size() == 1 && session->batch_inputs.size() == 1,
            "cached crops must not be prefetched again and singleton shapes must run only when requested")) {
        return false;
    }

    auto other_session = std::make_shared<CountingBatchOCRSession>();
    OCRer other_model(image, { first }, { }, other_session, "other-session", std::nullopt, &cache);
    OCRCache next_frame;
    OCRBatcher(image, compatible, session, next_frame);
    return check(
        other_model.best_result() && other_session->inputs.size() == 1 && session->batch_inputs.size() == 2,
        "prefetched results must remain isolated by session and screenshot cache");
}

bool test_batch_empty_and_fallback(const cv::Mat& image)
{
    const auto rois = batch_test_rois(4);
    const auto& first = rois[0];
    auto empty_session = std::make_shared<CountingBatchOCRSession>();
    empty_session->empty = true;
    OCRCache empty_cache;
    OCRBatcher(image, rois, empty_session, empty_cache);
    OCRer empty(image, rois, { }, empty_session, "empty-batch", std::nullopt, &empty_cache);
    if (!check(
            empty_session->batch_inputs.size() == 1 && empty_session->inputs.empty() && empty.all_results().empty()
                && std::ranges::all_of(rois, [&](const cv::Rect& roi) { return empty_cache.find(empty_session, roi); }),
            "successful empty batch items must be cached without solo retries")) {
        return false;
    }

    for (int failure = 0; failure < 3; ++failure) {
        auto session = std::make_shared<CountingBatchOCRSession>();
        session->fail_batch = failure == 0;
        session->malformed_batch = failure == 1;
        session->wrong_batch_size = failure == 2;
        OCRCache cache;
        OCRBatcher(image, rois, session, cache);
        if (!check(
                session->batch_inputs.size() == 1 && session->inputs.empty() && !cache.find(session, first),
                "failed or malformed batch items must remain uncached for lazy fallback")) {
            return false;
        }
        for (size_t i = 1; i < rois.size(); ++i) {
            if (!check(
                    static_cast<bool>(cache.find(session, rois[i])) == (failure == 1),
                    "a malformed item must not discard valid peers, while failed batches must cache no peers")) {
                return false;
            }
        }
        OCRer retried(image, { first }, { }, session, "batch-fallback", std::nullopt, &cache);
        OCRer reused(image, { first }, { }, session, "fallback-reused", std::nullopt, &cache);
        if (!check(
                retried.best_result() && details(reused) == details(retried) && session->inputs.size() == 1,
                "a failed batch item must retry through solo inference and cache its successful result")) {
            return false;
        }
    }

    auto unsupported = std::make_shared<CountingOCRSession>();
    OCRCache cache;
    OCRBatcher(image, rois, unsupported, cache);
    OCRer solo(image, { first }, { }, unsupported, "unsupported-batch", std::nullopt, &cache);
    if (!check(
            solo.best_result() && unsupported->inputs.size() == 1 && !cache.find(unsupported, rois[1]),
            "sessions without tensor batch support must preserve lazy solo inference")) {
        return false;
    }
    auto uncached = std::make_shared<CountingBatchOCRSession>();
    uncached->cache_supported = false;
    OCRBatcher(image, rois, uncached, cache);
    return check(
        uncached->batch_inputs.empty() && !cache.find(uncached, first) && !cache.find(uncached, rois[1]),
        "sessions that disable caching must not be prefetched");
}

bool test_batch_limits(const cv::Mat& image)
{
    for (size_t count : { 4, 5, 6, 8 }) {
        const auto rois = batch_test_rois(count);
        auto session = std::make_shared<CountingBatchOCRSession>();
        OCRCache cache;
        OCRBatcher(image, rois, session, cache);
        if (!check(
                session->batch_inputs.size() == count / 4 && session->inputs.empty(),
                "compatible groups must form batches of four without prefetching incomplete tails")) {
            return false;
        }
        for (const auto& batch : session->batch_inputs) {
            if (!check(batch.size() == 4, "each actual sub-batch must meet the four-ROI threshold")) {
                return false;
            }
        }
        for (size_t i = count / 4 * 4; i < count; ++i) {
            if (!check(!cache.find(session, rois[i]), "incomplete batch tails must remain uncached until requested")) {
                return false;
            }
        }
        OCRer all(image, rois, { }, session, "bounded-batch", std::nullopt, &cache);
        if (!check(
                session->inputs.size() == count % 4 && all.all_results().size() == 2 * rois.size(),
                "incomplete tails must use solo inference without losing any ROI result")) {
            return false;
        }
    }

    const auto rois = batch_test_rois(8);
    auto large_session = std::make_shared<CountingBatchOCRSession>();
    large_session->fixed_shape = cv::Size(960, 960);
    OCRCache cache;
    OCRBatcher(image, rois, large_session, cache);
    if (!check(
            large_session->batch_inputs.empty() && large_session->inputs.empty(),
            "the pixel budget must prevent batching when fewer than four crops fit, even for a large group")) {
        return false;
    }
    OCRer all(image, rois, { }, large_session, "over-budget-batch", std::nullopt, &cache);
    return check(
        large_session->inputs.size() == rois.size() && all.all_results().size() == 2 * rois.size(),
        "groups exceeding the pixel budget must retain independent on-demand inference");
}
} // namespace

bool test_ocr_cache()
{
    cv::Mat image(160, 240, CV_8UC3);
    cv::RNG rng(1448);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    if (!test_filters_and_rois(image) || !test_empty_failure_and_lifetime(image) || !test_incompatible_requests(image)
        || !test_batch_threshold(image) || !test_batch_prefetch_and_mapping(image) || !test_batch_empty_and_fallback(image)
        || !test_batch_limits(image)) {
        return false;
    }
    std::cout << "OCR cache: exact crops, tensor batching, independent filters, session isolation and failure handling passed\n";
    return true;
}
