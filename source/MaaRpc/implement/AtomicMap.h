#pragma once

#include <map>
#include <mutex>
#include <string>

template <typename Handle>
struct AtomicMap
{
    using HandleType = Handle;

    std::map<std::string, Handle> handlers;
    std::mutex mtx;

    bool add(const std::string& id, Handle handle)
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

    bool has(const std::string& id)
    {
        std::unique_lock<std::mutex> lock(mtx);
        return handlers.contains(id);
    }

    bool get(const std::string& id, Handle& handle)
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

    bool del(const std::string& id, Handle& handle)
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

    bool find(Handle handle, std::string& id)
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
};
