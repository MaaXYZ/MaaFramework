#pragma once

#include <functional>
#include <memory>
#include <unordered_set>

#include "Conf/Conf.h"

MAA_NS_BEGIN

template <typename T>
class Dispatcher
{
public:
    virtual ~Dispatcher() = default;

public:
    bool register_observer(const std::shared_ptr<T>& observer)
    {
        if (!observer) {
            return false;
        }
        return observers_.emplace(observer).second;
    }

    bool unregister_observer(const std::shared_ptr<T>& observer)
    {
        if (!observer) {
            return false;
        }
        return observers_.erase(observer) > 0;
    }

    void clear_observer() { observers_.clear(); }

    void dispatch(const std::function<void(const std::shared_ptr<T>&)>& pred)
    {
        if (!pred) {
            return;
        }

        for (auto& elem : observers_) {
            if (!elem) {
                continue;
            }

            pred(elem);
        }
    }

private:
    std::unordered_set<std::shared_ptr<T>> observers_;
};

MAA_NS_END
