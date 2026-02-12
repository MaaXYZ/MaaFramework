#pragma once

#ifndef __ANDROID__

#include "Base/UnitBase.h"
#include "MaaUtils/IOStream/ChildPipeIOStream.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "MaaUtils/Platform.h"

#include <optional>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class AVDExtras : public ScreencapBase
{
public:
    AVDExtras() = default;
    virtual ~AVDExtras() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool start();
    void stop();

    ProcessArgvGenerator emu_webrtc_start_argv_;
    ProcessArgvGenerator emu_webrtc_stop_argv_;

    bool inited_ = false;

    std::string shm_name_;
#ifdef _WIN32
    void* shm_handle_ = nullptr;
#else
    int shm_fd_ = -1;
    size_t shm_size_ = 0;
#endif
    void* shm_ = nullptr;

    // https://android.googlesource.com/platform/external/qemu/+/7d785f50047e1de08952811aff552ccbfa2449c4/android/android-ui/modules/aemu-recording/src/android/recording/video/VideoFrameSharer.h#53
    struct VideoInfo
    {
        uint32_t width;
        uint32_t height;
        uint32_t fps;  // target framerate (always 60)
        uint32_t frameNumber;
        uint64_t tsUs; // timestamp when this frame was captured
    };
};

MAA_CTRL_UNIT_NS_END

#endif
