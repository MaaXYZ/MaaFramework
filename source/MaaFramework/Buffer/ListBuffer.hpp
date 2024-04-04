#pragma once

#include <vector>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"

MAA_NS_BEGIN

template <typename T>
struct ListBuffer
{
public:
    virtual ~ListBuffer() = default;

    virtual bool empty() const { return list_.empty(); }

    virtual void clear() { list_.clear(); }

    virtual size_t size() const { return list_.size(); }

    virtual const T& at(size_t index) const { return list_.at(index); }

    virtual void append(T value) { list_.emplace_back(std::move(value)); }

    virtual void remove(size_t index) { list_.erase(list_.begin() + index); }

private:
    std::vector<T> list_;
};

MAA_NS_END