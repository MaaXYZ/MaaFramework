#pragma once

#include "Base/UnitBase.h"
#include "General/DeviceInfo.h"
#include "LibraryHolder/LibraryHolder.h"

#ifdef _WIN32
#include "Utils/SafeWindows.hpp"
#endif

namespace dnopengl
{
#ifndef _WIN32
#define HWND void*
#endif

#include "LD/dnopengl/dnopengl.h"

#ifndef _WIN32
#undef HWND
#endif
}

MAA_CTRL_UNIT_NS_BEGIN

class LDPlayerExtras
    : public LibraryHolder<LDPlayerExtras>
    , public ScreencapBase
{
public:
    LDPlayerExtras() { children_.emplace_back(device_info_); }

    virtual ~LDPlayerExtras() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    bool load_ld_library();
    bool create_ld_instance();
    void release_ld_instance();

private:
    std::filesystem::path lib_path_;

    unsigned int ld_index_ = 0;
    unsigned int ld_pid_ = 0;
    dnopengl::IScreenShotClass* ld_handle_ = nullptr;

    std::shared_ptr<DeviceInfo> device_info_ = std::make_shared<DeviceInfo>();

    int display_width_ = 0;
    int display_height_ = 0;

private:
    inline static const std::string kCreateInstanceFuncName = "CreateScreenShotInstance";
    boost::function<decltype(dnopengl::CreateScreenShotInstance)> create_instance_func_;
};

MAA_CTRL_UNIT_NS_END
