#pragma once

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MemfdBuffer
{
public:
    MemfdBuffer(uint32_t width, uint32_t height, uint32_t stride);
    ~MemfdBuffer();

    void* ptr() const { return ptr_; }

    uint32_t size() const { return height_ * stride_; }

    uint32_t width() const { return width_; }

    uint32_t height() const { return height_; }

    uint32_t stride() const { return stride_; }

    int fd() const { return fd_; }

    bool available() const { return available_; }

private:
    bool available_;
    int fd_;
    uint32_t width_;
    uint32_t height_;
    uint32_t stride_;
    void* ptr_;
};

MAA_CTRL_UNIT_NS_END
