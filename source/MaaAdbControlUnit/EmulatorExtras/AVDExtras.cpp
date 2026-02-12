#ifndef __ANDROID__

#include "AVDExtras.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"

#include <cerrno>
#include <cstdint>
#include <sstream>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

MAA_CTRL_UNIT_NS_BEGIN

AVDExtras::~AVDExtras()
{
    if (inited_) {
        inited_ = false;
        stop();
    }
}

bool AVDExtras::parse(const json::value& config)
{
    bool enable = config.get("extras", "avd", "enable", false);
    if (!enable) {
        LogInfo << "extras.avd.enable is false, ignore";
        return false;
    }

    static const json::array kDefaultEmuWebrtcStartArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "emu", "screenrecord webrtc start",
    };
    static const json::array kDefaultEmuWebrtcStopArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "emu", "screenrecord webrtc stop",
    };

    return parse_command("EmuWebrtcStart", config, kDefaultEmuWebrtcStartArgv, emu_webrtc_start_argv_)
           && parse_command("EmuWebrtcStop", config, kDefaultEmuWebrtcStopArgv, emu_webrtc_stop_argv_);
}

bool AVDExtras::init()
{
    inited_ = start();
    return inited_;
}

std::optional<cv::Mat> AVDExtras::screencap()
{
    if (shm_ == nullptr) {
        LogError << "AVDExtras shm_ is null";
        return std::nullopt;
    }

    VideoInfo* video_info = static_cast<VideoInfo*>(shm_);
    uint8_t* frame_data = static_cast<uint8_t*>(shm_) + sizeof(VideoInfo);
    cv::Mat bgra(video_info->height, video_info->width, CV_8UC4, frame_data);
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);

    return bgr;
}

bool AVDExtras::start()
{
    LogFunc;

    auto argv_opt = emu_webrtc_start_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return false;
    }

    // 成功的输出应为 "videmulator5554\r\nOK\r\n"
    std::istringstream iss(*output_opt);
    iss >> shm_name_;

    std::string status;
    iss >> status;
    if (status != "OK") {
        LogError << "Expected OK but got" << VAR(status);
        return false;
    }

    LogInfo << "AVDExtras shared memory name:" << VAR(shm_name_);

#ifdef _WIN32
    // https://android.googlesource.com/platform/external/qemu/+/7d785f50047e1de08952811aff552ccbfa2449c4/android/emu/base/memory/src/android/base/memory/SharedMemory_win32.cpp#37
    shm_name_ = "SHM_" + shm_name_;

    std::wstring wname(shm_name_.begin(), shm_name_.end());
    shm_handle_ = OpenFileMappingW(FILE_MAP_READ, false, wname.c_str());
    if (shm_handle_ == nullptr) {
        LogError << "AVDExtras failed to create file mapping:" << VAR(GetLastError());
        stop();
        return false;
    }

    shm_ = MapViewOfFile(shm_handle_, FILE_MAP_READ, 0, 0, 0);
    if (shm_ == nullptr) {
        LogError << "AVDExtras failed to map view of file:" << VAR(GetLastError());
        stop();
        return false;
    }
#else
    shm_fd_ = shm_open(shm_name_.c_str(), O_RDONLY, 0);
    if (shm_fd_ == -1) {
        LogError << "AVDExtras failed to open shared memory:" << VAR(errno) << VAR(strerror(errno));
        stop();
        return false;
    }

    struct stat st;
    if (fstat(shm_fd_, &st) == -1) {
        LogError << "AVDExtras failed to stat shared memory:" << VAR(errno) << VAR(strerror(errno));
        stop();
        return false;
    }
    shm_size_ = st.st_size;
    LogInfo << "AVDExtras shared memory size:" << VAR(shm_size_);

    shm_ = mmap(nullptr, shm_size_, PROT_READ, MAP_SHARED, shm_fd_, 0);
    if (shm_ == MAP_FAILED) {
        LogError << "AVDExtras failed to mmap shared memory:" << VAR(errno);
        shm_ = nullptr;
        stop();
        return false;
    }
#endif

    VideoInfo* video_info = static_cast<VideoInfo*>(shm_);
    LogInfo << "AVDExtras VideoInfo:" << VAR(video_info->width) << VAR(video_info->height) << VAR(video_info->fps)
            << VAR(video_info->frameNumber) << VAR(video_info->tsUs);

    return true;
}

void AVDExtras::stop()
{
    LogFunc;

    LogInfo << VAR(shm_name_);
#ifdef _WIN32
    if (shm_ != nullptr) {
        UnmapViewOfFile(shm_);
        shm_ = nullptr;
    }

    if (shm_handle_ != nullptr) {
        CloseHandle(shm_handle_);
        shm_handle_ = nullptr;
    }
#else
    if (shm_ != nullptr && shm_ != MAP_FAILED) {
        munmap(shm_, shm_size_);
        shm_ = nullptr;
    }

    if (shm_fd_ != -1) {
        close(shm_fd_);
        shm_fd_ = -1;
    }
#endif

    auto argv_opt = emu_webrtc_stop_argv_.gen(argv_replace_);
    if (argv_opt) {
        startup_and_read_pipe(*argv_opt);
    }
}

MAA_CTRL_UNIT_NS_END

#endif
