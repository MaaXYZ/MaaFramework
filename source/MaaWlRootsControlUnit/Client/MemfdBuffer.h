#pragma once

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MemfdBuffer
{
public:
    MemfdBuffer(int width, int height, int stride);
    ~MemfdBuffer();

    void* ptr() const { return ptr_; }

    int size() const { return height_ * stride_; }

    int width() const { return width_; }

    int height() const { return height_; }

    int stride() const { return stride_; }

    int fd() const { return fd_; }

    bool available() const { return available_; }

private:
    bool available_ = false;
    int fd_ = -1;
    int width_ = 0;
    int height_ = 0;
    int stride_ = 0;
    void* ptr_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
