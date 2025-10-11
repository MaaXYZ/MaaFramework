#pragma once

#include <functional>
#include <map>
#include <memory>
#include <ranges>

#include "Conf/Conf.h"

MAA_NS_BEGIN

template <typename SinkT>
class Dispatcher
{
public:
    using ObserverId = MaaId;

    virtual ~Dispatcher() = default;

public:
    ObserverId register_observer(const std::shared_ptr<SinkT>& observer)
    {
        if (!observer) {
            return MaaInvalidId;
        }
        auto it = observers_.emplace(++observer_id_, observer).first;
        if (it == observers_.end()) {
            return MaaInvalidId;
        }
        return observer_id_;
    }

    bool unregister_observer(ObserverId observer_id) { return observers_.erase(observer_id) > 0; }

    void clear_observer() { observers_.clear(); }

    void dispatch(const std::function<void(const std::shared_ptr<SinkT>&)>& pred)
    {
        if (!pred) {
            return;
        }

        for (auto& elem : observers_ | std::views::values) {
            if (!elem) {
                continue;
            }

            pred(elem);
        }
    }

private:
    std::map<ObserverId, std::shared_ptr<SinkT>> observers_;
    ObserverId observer_id_ = 0;
};

MAA_NS_END
