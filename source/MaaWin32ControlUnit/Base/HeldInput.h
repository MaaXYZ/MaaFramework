#pragma once

#include <set>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// Ownership belongs to the backend that issued the down. Keep failed releases
// for retry, and never synthesize an unmatched release after its target is lost.
class HeldInput
{
public:
    bool down(int code, bool succeeded)
    {
        if (succeeded) {
            held_.insert(code);
        }
        return succeeded;
    }

    bool contains(int code) const { return held_.contains(code); }

    template <typename Release>
    bool up(int code, bool target_available, Release release)
    {
        if ((!target_available && !contains(code)) || !release(code)) {
            return false;
        }
        held_.erase(code);
        return true;
    }

    template <typename Release>
    bool release_all(Release release)
    {
        std::erase_if(held_, [&](int code) { return release(code); });
        return held_.empty();
    }

private:
    std::set<int> held_;
};

MAA_CTRL_UNIT_NS_END
