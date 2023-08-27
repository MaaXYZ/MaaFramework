#pragma once

#include "API/MaaTypes.h"
#include "Utils/NoWarningCVMat.hpp"

#include <cstdlib>
#include <vector>

MAA_NS_BEGIN

class MaaBuffer : public MaaBufferAPI
{
public:
    struct move_in_tag
    {};

    MaaBuffer() : buffer(nullptr), size(0) {}
    MaaBuffer(const void* buf, MaaSize sz) { acquire(buf, sz); }
    MaaBuffer(void* buf, MaaSize sz, move_in_tag) : buffer(reinterpret_cast<uint8_t*>(buf)), size(sz) {}
    MaaBuffer(const MaaBufferRef& ref) : MaaBuffer(ref.buffer, ref.size, move_in_tag {}) {}
    MaaBuffer(const cv::Mat& mat)
    {
        auto size = sizeof(MaaImageBuffer) + mat.elemSize();
        buffer = new uint8_t[size];
        auto buf = new (buffer) MaaImageBuffer { mat.rows, mat.cols, mat.type() };
        memcpy(buf->buffer, mat.data, mat.elemSize());
    }

    MaaBuffer(const MaaBuffer& other) { acquire(other.buffer, other.size); }
    MaaBuffer(MaaBuffer&& other) : buffer(other.buffer), size(other.size)
    {
        other.buffer = nullptr;
        other.size = 0;
    }
    virtual ~MaaBuffer() override { release(); }
    MaaBuffer& operator=(const MaaBuffer& other)
    {
        if (this == &other) {
            return *this;
        }
        release();
        acquire(other.buffer, other.size);
        return *this;
    }
    MaaBuffer& operator=(MaaBuffer&& other)
    {
        if (this == &other) {
            return *this;
        }
        release();
        buffer = other.buffer;
        size = other.size;
        other.buffer = nullptr;
        other.size = 0;
        return *this;
    }

    MaaBufferRef takeRef()
    {
        MaaBufferRef ref { size, buffer };
        buffer = nullptr;
        size = 0;
        return ref;
    }
    cv::Mat toMat()
    {
        MaaImageBuffer* info = reinterpret_cast<MaaImageBuffer*>(buffer);
        return cv::Mat(info->rows, info->cols, info->type, info->buffer);
    }

    virtual MaaSize getSize() override { return size; }
    virtual void getContent(void* buf) override { memcpy(buf, buffer, size); }

private:
    void acquire(const void* buf, MaaSize sz)
    {
        size = sz;
        buffer = new uint8_t[size];
        memcpy(buffer, buf, size);
    }
    void release()
    {
        if (buffer) {
            delete[] buffer;
        }
    }

    uint8_t* buffer;
    MaaSize size;
};

MAA_NS_END
