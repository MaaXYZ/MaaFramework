#pragma once

#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCV.hpp"

namespace Ort
{
struct Session;
}

MAA_VISION_NS_BEGIN

// A cache belongs to one immutable screenshot. Keep the session alive so a
// reloaded model cannot reuse an old session address and hit stale results.
template <typename Result>
class InferenceCache
{
public:
    using Results = std::vector<Result>;

    const Results* find(const std::shared_ptr<Ort::Session>& session, const cv::Rect& roi) const
    {
        auto it = results_.find(make_key(session, roi));
        return it == results_.end() ? nullptr : &it->second;
    }

    void insert(const std::shared_ptr<Ort::Session>& session, const cv::Rect& roi, Results results)
    {
        results_.insert_or_assign(make_key(session, roi), std::move(results));
    }

private:
    using Key = std::tuple<std::shared_ptr<Ort::Session>, int, int, int, int>;

    static Key make_key(const std::shared_ptr<Ort::Session>& session, const cv::Rect& roi)
    {
        return { session, roi.x, roi.y, roi.width, roi.height };
    }

    std::map<Key, Results> results_;
};

MAA_VISION_NS_END
