#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

template <typename Handle, typename Key = std::string>
class AtomicMap
{
public:
    using KeyType = Key;
    using HandleType = Handle;

    bool add(const Key& id, Handle handle)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (handlers_.contains(id)) {
            return false;
        }

        handlers_.emplace(id, handle);
        return true;
    }

    bool has(const Key& id) const
    {
        std::unique_lock<std::mutex> lock(mtx_);
        return handlers_.contains(id);
    }

    bool get(const Key& id, Handle& handle) const
    {
        std::unique_lock<std::mutex> lock(mtx_);

        auto iter = handlers_.find(id);
        if (iter == handlers_.end()) {
            return false;
        }

        handle = iter->second;
        return true;
    }

    bool del(const Key& id, Handle& handle)
    {
        std::unique_lock<std::mutex> lock(mtx_);

        auto iter = handlers_.find(id);
        if (iter == handlers_.end()) {
            return false;
        }

        handle = iter->second;
        handlers_.erase(iter);
        return true;
    }

    bool del(const Key& id)
    {
        Handle _;
        return del(id, _);
    }

    bool find(Handle handle, Key& id) const
    {
        std::unique_lock<std::mutex> lock(mtx_);
        for (const auto& pr : handlers_) {
            if (pr.second == handle) {
                id = pr.first;
                return true;
            }
        }
        return false;
    }

    bool find_all(Handle handle, std::vector<Key>& ids) const
    {
        ids.clear();

        std::unique_lock<std::mutex> lock(mtx_);
        for (const auto& pr : handlers_) {
            if (pr.second == handle) {
                ids.push_back(pr.first);
            }
        }
        return !ids.empty();
    }

private:
    std::map<Key, Handle> handlers_;
    mutable std::mutex mtx_;
};
