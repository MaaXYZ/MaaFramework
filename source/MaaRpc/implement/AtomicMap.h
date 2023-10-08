#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

template <typename Handle, typename Key = std::string>
struct AtomicMap
{
    using KeyType = Key;
    using HandleType = Handle;

    std::map<Key, Handle> handlers;
    std::mutex mtx;

    bool add(const Key& id, Handle handle)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (handlers.contains(id)) {
            return false;
        }
        else {
            handlers[id] = handle;
            return true;
        }
    }

    bool has(const Key& id)
    {
        std::unique_lock<std::mutex> lock(mtx);
        return handlers.contains(id);
    }

    bool get(const Key& id, Handle& handle)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!handlers.contains(id)) {
            return false;
        }
        else {
            handle = handlers[id];
            return true;
        }
    }

    bool del(const Key& id, Handle& handle)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!handlers.contains(id)) {
            return false;
        }
        else {
            handle = handlers[id];
            handlers.erase(id);
            return true;
        }
    }

    bool del(const Key& id)
    {
        Handle _;
        return del(id, _);
    }

    bool find(Handle handle, Key& id)
    {
        std::unique_lock<std::mutex> lock(mtx);
        for (const auto& pr : handlers) {
            if (pr.second == handle) {
                id = pr.first;
                return true;
            }
        }
        return false;
    }

    bool find_all(Handle handle, std::vector<Key>& id)
    {
        id.clear();

        std::unique_lock<std::mutex> lock(mtx);
        for (const auto& pr : handlers) {
            if (pr.second == handle) {
                id.push_back(pr.first);
            }
        }
        return id.size() > 0;
    }
};
