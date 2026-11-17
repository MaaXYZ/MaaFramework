#include "OCRCache.h"

#include <iostream>

#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/OCRer.h"

namespace
{
using namespace MAA_VISION_NS;

class CountingOCRSession : public OCRSession
{
public:
    int det_max_side_len() const override { return 960; }

    bool supports_batch_cache() const override { return cache_supported; }

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
            return OCRDetRecResult {};
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

    param = {};
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
    lifetime_cache = {};
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
} // namespace

bool test_ocr_cache()
{
    cv::Mat image(160, 240, CV_8UC3);
    cv::RNG rng(1448);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    if (!test_filters_and_rois(image) || !test_empty_failure_and_lifetime(image) || !test_incompatible_requests(image)) {
        return false;
    }
    std::cout << "OCR cache: exact crops, independent filters, session isolation and failure handling passed\n";
    return true;
}
