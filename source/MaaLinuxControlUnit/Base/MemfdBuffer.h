#pragma once

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MemfdBuffer
{
public:
    MemfdBuffer(int size);
    MemfdBuffer(int width, int height, int stride);
    ~MemfdBuffer();

    void* ptr() const { return ptr_; }

    int size() const { return size_; }

    int fd() const { return fd_; }

    bool available() const { return available_; }

private:
    bool create_buffer();
    bool available_ = false;
    int fd_ = -1;
    int size_ = 0;
    void* ptr_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
