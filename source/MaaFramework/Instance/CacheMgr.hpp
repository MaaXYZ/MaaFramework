#pragma once

#include <list>
#include <optional>
#include <ranges>
#include <utility>

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

template <typename Key, typename Item, size_t CacheSize = 100, typename Equal = std::equal_to<Key>>
class CacheMgr : public NonCopyable
{
public:
    CacheMgr() = default;
    CacheMgr(CacheMgr&&) = default;
    CacheMgr(Equal equal)
        : equal_(std::move(equal))
    {
    }

    std::optional<Item> get_cache(const Key& key) const;
    void set_cache(Key key, Item item);
    void clear();

private:
    std::list<std::pair<Key, Item>> cache_;
    Equal equal_ {};
};

template <typename Key, typename Item, size_t CacheSize, typename Equal>
std::optional<Item> CacheMgr<Key, Item, CacheSize, Equal>::get_cache(const Key& key) const
{
    auto it =
        std::ranges::find_if(cache_, [&](const auto& pair) { return equal_(pair.first, key); });
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second;
}

template <typename Key, typename Item, size_t CacheSize, typename Equal>
void CacheMgr<Key, Item, CacheSize, Equal>::set_cache(Key key, Item item)
{
    if (cache_.size() >= CacheSize) {
        cache_.pop_front();
    }
    cache_.emplace_back(std::make_pair(std::move(key), std::move(item)));
}

template <typename Key, typename Item, size_t CacheSize, typename Equal>
void CacheMgr<Key, Item, CacheSize, Equal>::clear()
{
    cache_.clear();
}

MAA_NS_END