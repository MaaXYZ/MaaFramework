#include "MemfdBuffer.h"

#include "MaaUtils/Logger.h"

#include <sys/mman.h>

#define BUFFER_NAME "maafw-wl-buffer"

MAA_CTRL_UNIT_NS_BEGIN

MemfdBuffer::MemfdBuffer(int width, int height, int stride)
    : width_(width)
    , height_(height)
    , stride_(stride)
{
    LogDebug << "Creating new shm buffer" << VAR(width) << VAR(height) << VAR(stride);

    fd_ = memfd_create(BUFFER_NAME, 0);
    if (fd_ < 0) {
        LogError << "Failed to create memfd";
        return;
    }
    if (int ret = ftruncate(fd_, size()); ret < 0) {
        LogError << "Failed to allocate buffer";
        close(fd_);
        fd_ = 0;
        return;
    }
    ptr_ = mmap(nullptr, size(), PROT_READ, MAP_SHARED, fd_, 0);
    if (ptr_ == MAP_FAILED) {
        LogError << "Failed to mmap buffer";
        ptr_ = nullptr;
        return;
    }
    available_ = true;
}

MemfdBuffer::~MemfdBuffer()
{
    if (!fd_) {
        return;
    }
    LogDebug << "Closing buffer" << VAR(fd_);
    if (ptr_) {
        munmap(ptr_, size());
        ptr_ = nullptr;
    }
    close(fd_);
}

MAA_CTRL_UNIT_NS_END
