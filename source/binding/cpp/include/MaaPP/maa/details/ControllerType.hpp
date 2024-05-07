// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <cstdint>

namespace maa::details
{

template <
    typename Type,
    uint32_t TouchMask,
    uint32_t KeyMask,
    uint32_t ScreencapMask,
    typename IType>
struct ControllerType
{
    Type type_ = 0;

    ControllerType(Type type = 0)
        : type_(type)
    {
    }

    IType& set_touch(uint32_t touch)
    {
        type_ = (type_ & ~(TouchMask)) | (touch & TouchMask);
        return static_cast<IType&>(*this);
    }

    uint32_t touch() const { return type_ & TouchMask; }

    IType& set_key(uint32_t key)
    {
        type_ = (type_ & ~(KeyMask)) | (key & KeyMask);
        return static_cast<IType&>(*this);
    }

    uint32_t key() const { return type_ & KeyMask; }

    IType& set_screencap(uint32_t screencap)
    {
        type_ = (type_ & ~(ScreencapMask)) | (screencap & ScreencapMask);
        return static_cast<IType&>(*this);
    }

    uint32_t screencap() const { return type_ & ScreencapMask; }

    operator Type() const { return type_; }
};

}
