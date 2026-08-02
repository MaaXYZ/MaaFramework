#pragma once

#include <ostream>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Common/Conf.h"
#include "MaaUtils/Buffer/ListBuffer.hpp"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

struct DesktopWindow
{
    void* hwnd = nullptr;
    std::wstring class_name;
    std::wstring window_name;
    std::wstring process_path;

    MEO_TOJSON(hwnd, class_name, window_name, process_path);
};

class DesktopWindowBuffer : public MaaToolkitDesktopWindow
{
public:
    DesktopWindowBuffer(const DesktopWindow& window)
        : hwnd_(window.hwnd)
        , class_name_(from_u16(window.class_name))
        , window_name_(from_u16(window.window_name))
        , process_path_(from_u16(window.process_path))
    {
    }

    virtual ~DesktopWindowBuffer() override = default;

    virtual void* handle() const override { return hwnd_; }

    virtual const std::string& class_name() const override { return class_name_; }

    virtual const std::string& window_name() const override { return window_name_; }

    virtual const std::string& process_path() const override { return process_path_; }

private:
    void* hwnd_ = nullptr;
    std::string class_name_;
    std::string window_name_;
    std::string process_path_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitDesktopWindowList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::DesktopWindowBuffer>
{
    virtual ~MaaToolkitDesktopWindowList() override = default;
};
