#pragma once

#include "API/MaaTypes.h"
#include "Conf/Conf.h"

MAA_NS_BEGIN

class StringBuffer : public MaaStringBuffer
{
public:
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
