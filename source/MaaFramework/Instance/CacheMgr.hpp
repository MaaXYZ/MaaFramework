#pragma once

#include <list>
#include <optional>
#include <ranges>
#include <utility>

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

template <typename Key, typename Item, size_t tCacheSize = 100, typename Pred = std::equal_to<Key>>
class CacheMgr : public NonCopyable
{
public:
    static constexpr size_t kCacheSize = tCacheSize;

public:
    CacheMgr() = default;
    CacheMgr(CacheMgr&&) = default;
    CacheMgr(Pred pred) : pred_(std::move(pred)) {}

    std::optional<Item> get_cache(const Key& key) const;
    void set_cache(Key key, Item item);
    void clear();

private:
    std::list<std::pair<Key, Item>> cache_;
    Pred pred_ {};
};

template <typename Key, typename Item, size_t tCacheSize, typename Pred>
std::optional<Item> CacheMgr<Key, Item, tCacheSize, Pred>::get_cache(const Key& key) const
{
    auto it = std::ranges::find_if(cache_, [&](const auto& pair) { return pred_(pair.first, key); });
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second;
}

template <typename Key, typename Item, size_t tCacheSize, typename Pred>
void CacheMgr<Key, Item, tCacheSize, Pred>::set_cache(Key key, Item item)
{
    if (cache_.size() >= kCacheSize) {
        cache_.pop_front();
    }
    cache_.emplace_back(std::make_pair(std::move(key), std::move(item)));
}

template <typename Key, typename Item, size_t tCacheSize, typename Pred>
void CacheMgr<Key, Item, tCacheSize, Pred>::clear()
{
    cache_.clear();
}

MAA_NS_END
