#pragma once

#include <cstdint>

namespace maa::details
{

template <typename Type, uint32_t TouchMask, uint32_t KeyMask, uint32_t ScreencapMask>
struct ControllerType
{
    Type type_;

    ControllerType(Type type = 0)
        : type_(type)
    {
    }

    void set_touch(uint32_t touch) { type_ = (type_ & ~(TouchMask)) | (touch & TouchMask); }

    uint32_t touch() const { return type_ & TouchMask; }

    void set_key(uint32_t key) { type_ = (type_ & ~(KeyMask)) | (key & KeyMask); }

    uint32_t key() const { return type_ & KeyMask; }

    void set_screencap(uint32_t screencap)
    {
        type_ = (type_ & ~(ScreencapMask)) | (screencap & ScreencapMask);
    }

    uint32_t screencap() const { return type_ & ScreencapMask; }

    operator Type() const { return type_; }
};

}
