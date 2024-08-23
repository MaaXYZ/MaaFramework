#pragma once

#include <ostream>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Buffer/ListBuffer.hpp"
#include "Conf/Conf.h"
#include "Utils/Codec.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

struct DesktopWindow
{
    void* hwnd = nullptr;
    std::wstring class_name;
    std::wstring window_name;
};

inline std::ostream& operator<<(std::ostream& os, const DesktopWindow& window)
{
    return os << VAR_VOIDP_RAW(window.hwnd) << VAR_RAW(from_u16(window.class_name)) << VAR_RAW(from_u16(window.window_name));
}

class DesktopWindowBuffer : public MaaToolkitDesktopWindow
{
public:
    DesktopWindowBuffer(const DesktopWindow& window)
        : hwnd_(window.hwnd)
        , class_name_(from_u16(window.class_name))
        , window_name_(from_u16(window.window_name))
    {
    }

    virtual ~DesktopWindowBuffer() override = default;

    virtual void* handle() const override { return hwnd_; }

    virtual const std::string& class_name() const override { return class_name_; }

    virtual const std::string& window_name() const override { return window_name_; }

private:
    void* hwnd_ = nullptr;
    std::string class_name_;
    std::string window_name_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitDesktopWindowList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::DesktopWindowBuffer>
{
    virtual ~MaaToolkitDesktopWindowList() override = default;
};
