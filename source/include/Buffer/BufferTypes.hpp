#pragma once

#include <stdint.h>

#include <string>

#include "Utils/NoWarningCVMat.hpp"

struct MaaStringBuffer
{
public:
    virtual ~MaaStringBuffer() = default;

    virtual bool empty() const = 0;
    virtual void clear() = 0;

    virtual const char* data() const = 0;
    virtual size_t size() const = 0;

    virtual const std::string& get() const = 0;

    virtual void set(std::string str) = 0;
};

struct MaaImageBuffer
{
public:
    virtual ~MaaImageBuffer() = default;

    virtual bool empty() const = 0;
    virtual void clear() = 0;

    virtual void* raw_data() const = 0;
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
    virtual int32_t channles() const = 0;
    virtual int32_t type() const = 0;

    virtual uint8_t* encoded() const = 0;
    virtual size_t encoded_size() const = 0;

    virtual const cv::Mat& get() const = 0;

    virtual void set(cv::Mat image) = 0;
};
