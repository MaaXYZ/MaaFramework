#include "MemfdBuffer.h"

#include "MaaUtils/Logger.h"
#include "WaylandHelper.h"

#include <sys/mman.h>

#define BUFFER_NAME "maafw-wl-buffer-XXXXXX"

MAA_CTRL_UNIT_NS_BEGIN

MemfdBuffer::MemfdBuffer(int size)
    : size_(size)
{
    LogDebug << "Creating new shm buffer" << VAR(size);
    if (create_buffer()) {
        available_ = true;
    }
    else {
        LogError << "Failed to create shm buffer";
    }
}

MemfdBuffer::MemfdBuffer(int width, int height, int stride)
    : size_(height * stride)
{
    LogDebug << "Creating new shm buffer" << VAR(width) << VAR(height) << VAR(stride);
    if (create_buffer()) {
        available_ = true;
    }
    else {
        LogError << "Failed to create shm buffer";
    }
}

MemfdBuffer::~MemfdBuffer()
{
    if (!fd_) {
        return;
    }
    LogDebug << "Closing buffer" << VAR(fd_);
    if (ptr_) {
        munmap(ptr_, size_);
        ptr_ = nullptr;
    }
    close(fd_);
}

bool MemfdBuffer::create_buffer()
{
    std::string name = BUFFER_NAME;
    WaylandHelper::randname(name);
    fd_ = memfd_create(name.c_str(), 0);
    if (fd_ < 0) {
        LogError << "Failed to create memfd";
        return false;
    }
    if (const int ret = ftruncate(fd_, size_); ret < 0) {
        LogError << "Failed to allocate buffer";
        close(fd_);
        fd_ = -1;
        return false;
    }
    ptr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr_ == MAP_FAILED) {
        LogError << "Failed to mmap buffer";
        ptr_ = nullptr;
        return false;
    }
    LogDebug << "Created new buffer" << VAR(fd_);
    return true;
}

MAA_CTRL_UNIT_NS_END
