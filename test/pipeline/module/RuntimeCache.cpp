#include "RuntimeCache.h"

#include <iostream>
#include <shared_mutex>
#include <utility>

#include "Global/OptionMgr.h"
#include "Tasker/RuntimeCache.h"

MAA_NS_BEGIN

class RuntimeCacheTest
{
public:
    static bool run()
    {
        const size_t original_limit = MAA_GLOBAL_NS::OptionMgr::get_instance().reco_image_cache_limit();
        const bool passed = test_disabled() && test_bounded() && test_disable_existing() && test_instance_budgets();
        const bool restored = set_limit(original_limit);
        if (!passed || !restored) {
            return false;
        }
        std::cout << "RuntimeCache: image storage release, bounded retention, option changes and clear passed\n";
        return true;
    }

private:
    static bool check(bool condition, const char* message, MaaRecoId id = MaaInvalidId)
    {
        if (!condition) {
            std::cerr << "RuntimeCache: " << message << " (reco_id=" << id << ")\n";
        }
        return condition;
    }

    static bool set_limit(size_t limit)
    {
        return check(
            MAA_GLOBAL_NS::OptionMgr::get_instance().set_option(MaaGlobalOption_RecoImageCacheLimit, &limit, sizeof(limit)),
            "failed to set image cache limit");
    }

    static MAA_TASK_NS::RecoResult make_result(MaaRecoId id)
    {
        MAA_TASK_NS::RecoResult result {
            .reco_id = id,
            .name = "CacheProbe",
            .algorithm = "Custom",
            .box = cv::Rect(1, 2, 3, 4),
            .detail = json::object { { "marker", id } },
            .raw = { 1, 2, 3, 4 },
            .draws = { { 5, 6 }, { 7, 8, 9 } },
        };
        result.raw.reserve(4096);
        result.draws.reserve(8);
        return result;
    }

    static bool metadata_released_storage(const RuntimeCache& cache, MaaRecoId id)
    {
        std::shared_lock lock(cache.reco_details_mutex_);
        const auto it = cache.reco_details_.find(id);
        // A getter copy discards empty vectors' capacity and would hide this regression.
        return check(
            it != cache.reco_details_.end() && it->second.raw.capacity() == 0 && it->second.draws.capacity() == 0,
            "recognition metadata retained image allocation",
            id);
    }

    static bool result_matches(const RuntimeCache& cache, MaaRecoId id, bool images)
    {
        const auto result = cache.get_reco_result(id);
        const auto expected = make_result(id);
        if (!check(
                result && result->reco_id == id && result->name == expected.name && result->algorithm == expected.algorithm
                    && result->box == expected.box && result->detail == expected.detail,
                "recognition metadata changed",
                id)) {
            return false;
        }
        return check(
            images ? result->raw == expected.raw && result->draws == expected.draws : result->raw.empty() && result->draws.empty(),
            "unexpected recognition images",
            id);
    }

    static bool store_empty_with_capacity(RuntimeCache& cache, MaaRecoId id)
    {
        auto result = make_result(id);
        result.raw.clear();
        result.draws.clear();
        if (!check(result.raw.capacity() > 0 && result.draws.capacity() > 0, "empty fixture lost reserved storage", id)) {
            return false;
        }
        cache.set_reco_detail(id, std::move(result));
        return metadata_released_storage(cache, id) && result_matches(cache, id, false);
    }

    static bool test_disabled()
    {
        if (!set_limit(0)) {
            return false;
        }
        RuntimeCache cache;
        cache.set_reco_detail(101, make_result(101));
        return metadata_released_storage(cache, 101) && result_matches(cache, 101, false) && store_empty_with_capacity(cache, 102);
    }

    static bool test_bounded()
    {
        if (!set_limit(2)) {
            return false;
        }
        RuntimeCache cache;
        for (MaaRecoId id : { 201, 202, 203 }) {
            cache.set_reco_detail(id, make_result(id));
            if (!metadata_released_storage(cache, id) || !result_matches(cache, id, true)) {
                return false;
            }
        }
        if (!result_matches(cache, 201, false) || !result_matches(cache, 202, true) || !result_matches(cache, 203, true)) {
            return false;
        }
        cache.clear({ });
        return check(
            !cache.get_reco_result(201) && !cache.get_reco_result(202) && !cache.get_reco_result(203),
            "clear retained recognition history");
    }

    static bool test_instance_budgets()
    {
        if (!set_limit(0)) {
            return false;
        }
        RuntimeCache first;
        RuntimeCache second;
        first.set_image_limit(MaaTaskerOption_RecoImageCacheLimit, 10);
        first.set_image_limit(MaaTaskerOption_RecoImageCacheBytes, 18);
        for (MaaRecoId id : { 401, 402, 403 }) {
            first.set_reco_detail(id, make_result(id));
            second.set_reco_detail(id, make_result(id));
        }
        if (!result_matches(first, 401, false) || !result_matches(first, 402, true) || !result_matches(first, 403, true)
            || !result_matches(second, 403, false)
            || !check(first.get_reco_result(401)->image_status == "evicted", "eviction reason missing")
            || !check(first.reco_image_bytes_ == 18, "byte budget accounting incorrect")) {
            return false;
        }
        auto large = make_result(404);
        large.raw.resize(32);
        if (!check(first.set_reco_detail(404, std::move(large)) == "budget_exceeded", "oversize entry accepted")
            || !result_matches(first, 402, true) || !result_matches(first, 403, true)) {
            return false;
        }
        first.set_image_limit(MaaTaskerOption_RecoImageCacheBytes, 9);
        if (!result_matches(first, 402, false) || !result_matches(first, 403, true)) {
            return false;
        }
        // Replacing an ID must not leave duplicate FIFO entries or double-count bytes.
        first.set_reco_detail(403, make_result(403));
        if (!check(first.reco_image_bytes_ == 9 && first.reco_image_order_.size() == 1, "replacement duplicated storage")) {
            return false;
        }
        first.clear({});
        return check(first.reco_image_bytes_ == 0, "clear retained byte accounting");
    }

    static bool test_disable_existing()
    {
        if (!set_limit(2)) {
            return false;
        }
        RuntimeCache cache;
        cache.set_reco_detail(301, make_result(301));
        if (!metadata_released_storage(cache, 301) || !set_limit(0) || !result_matches(cache, 301, true)) {
            return false;
        }
        cache.set_reco_detail(302, make_result(302));
        if (!metadata_released_storage(cache, 302) || !result_matches(cache, 302, false) || !store_empty_with_capacity(cache, 303)
            || !result_matches(cache, 301, true)) {
            return false;
        }
        cache.clear({ });
        return check(
            !cache.get_reco_result(301) && !cache.get_reco_result(302) && !cache.get_reco_result(303),
            "clear retained images after disabling caching");
    }
};

MAA_NS_END

bool test_runtime_cache()
{
    return MAA_NS::RuntimeCacheTest::run();
}
