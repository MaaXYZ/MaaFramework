#pragma once

#include "BufferTypes.hpp"
#include "Conf/Conf.h"
#include "ListBuffer.hpp"

MAA_NS_BEGIN

class StringBuffer : public MaaStringBuffer
{
public:
    StringBuffer() = default;

    StringBuffer(std::string str)
        : str_(std::move(str))
    {
    }

    virtual ~StringBuffer() override = default;

    virtual bool empty() const override { return str_.empty(); }

    virtual void clear() override { str_.clear(); }

    virtual const char* data() const override { return str_.data(); }

    virtual size_t size() const override { return str_.size(); }

    virtual const std::string& get() const override { return str_; }

    virtual void set(std::string str) override { str_ = std::move(str); }

private:
    std::string str_;
};

MAA_NS_END

struct MaaStringListBuffer : public MAA_NS::ListBuffer<MAA_NS::StringBuffer>
{
    virtual ~MaaStringListBuffer() override = default;
};
