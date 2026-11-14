#pragma once

#ifndef __ANDROID__

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

#include "Base/UnitBase.h"
#include "Input/AdbShellInput.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// ModAVDExtras uses the modavd_extras library shipped with ModAVD (<ModAVD root>/extras/) to capture the screen
// from the emulator's shared frame and to send touches, keys and text through the emulator console.
// The library never falls back to adb by itself; keys and text fall back to AdbShellInput here when the library
// or the instance does not support them.
// API: https://github.com/ModAVD/ModAVDExtras/blob/main/include/modavd_extras.h
class ModAVDExtras
    : public ScreencapBase
    , public InputBase
{
public:
    enum class Usage
    {
        Screencap,
        Input,
    };

public:
    explicit ModAVDExtras(Usage usage);
    virtual ~ModAVDExtras() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase / InputBase
    virtual bool init() override;

    virtual std::optional<cv::Mat> screencap() override;

public: // from InputBase
    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

public: // from ControlUnitSink
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;

private:
    // Not derived from LibraryHolder: its destructor always drops a reference, and the count is shared by every
    // instance of the type. ScreencapAgent and InputAgent both construct this unit even when it is not enabled.
    using Library = LibraryHolder<ModAVDExtras>;

    bool load_modavd_library();
    void unload_modavd_library();
    bool open_instance();
    void close_instance();
    bool reopen_instance();
    // ret: the library's last error code; kOk when the failure did not come from the library
    std::optional<cv::Mat> capture_frame(int& ret);
    std::optional<cv::Mat> capture_once(int& ret);
    std::string error_string(int ret) const;

    // Drops a capability after the library said MODAVD_E_UNSUPPORTED, so later calls go to adb directly.
    void drop_capability_if_unsupported(int ret, uint32_t cap);

private:
    const Usage usage_;

    bool enabled_ = false;
    std::filesystem::path lib_path_;
    std::string address_;
    int console_port_ = 0;

    bool library_loaded_ = false;
    uint32_t api_version_ = 0;
    void* handle_ = nullptr;
    uint32_t capabilities_ = 0;
    // MODAVD_CAP_* the library answered MODAVD_E_UNSUPPORTED for; kept across reopens.
    uint32_t unsupported_ = 0;

    // Keys sent with adb because the library failed on key_down; their key_up is a no-op.
    std::unordered_set<int> adb_clicked_keys_;

    std::shared_ptr<AdbShellInput> adb_shell_input_ = nullptr;

private:
    // Same layout as modavd_open_params / modavd_frame_info in modavd_extras.h; the size field keeps
    // older and newer libraries compatible.
    struct OpenParams
    {
        uint32_t size;
        const char* adb_address;
        int console_port;
        const char* client_name;
        uint32_t flags;
        int timeout_ms;
    };

    struct FrameInfo
    {
        uint32_t size;
        int32_t width;
        int32_t height;
        int32_t stride;
        uint32_t format;
        uint32_t frame_number;
        uint64_t timestamp_us;
    };

    static constexpr int kOk = 0;
    static constexpr int kErrNotReady = -3;
    static constexpr int kErrUnsupported = -7;
    static constexpr int kNotReadyRetries = 20;
    static constexpr uint32_t kCapScreencap = 1u << 0;
    static constexpr uint32_t kCapTouch = 1u << 1;
    static constexpr uint32_t kCapKey = 1u << 2;
    static constexpr uint32_t kCapText = 1u << 3;
    static constexpr uint32_t kFormatBgr = 2;

    using ApiVersionFunc = uint32_t();
    using OpenFunc = int(const OpenParams*, void**);
    using CloseFunc = void(void*);
    using CapabilitiesFunc = uint32_t(void*);
    using CaptureFunc = int(void*, int, uint32_t, uint32_t, void*, size_t, FrameInfo*);
    using StrErrorFunc = const char*(int);
    using TouchFunc = int(void*, int, int, int, int, int);
    using TouchUpFunc = int(void*, int, int);
    using KeyFunc = int(void*, int, int, int);
    using InputTextFunc = int(void*, const char*);

    inline static const std::string kApiVersionFuncName = "modavd_api_version";
    inline static const std::string kOpenFuncName = "modavd_open";
    inline static const std::string kCloseFuncName = "modavd_close";
    inline static const std::string kCapabilitiesFuncName = "modavd_capabilities";
    inline static const std::string kCaptureFuncName = "modavd_capture";
    inline static const std::string kStrErrorFuncName = "modavd_strerror";
    inline static const std::string kTouchDownFuncName = "modavd_touch_down";
    inline static const std::string kTouchMoveFuncName = "modavd_touch_move";
    inline static const std::string kTouchUpFuncName = "modavd_touch_up";
    inline static const std::string kKeyFuncName = "modavd_key";
    inline static const std::string kInputTextFuncName = "modavd_input_text";

    boost::function<ApiVersionFunc> api_version_func_;
    boost::function<OpenFunc> open_func_;
    boost::function<CloseFunc> close_func_;
    boost::function<CapabilitiesFunc> capabilities_func_;
    boost::function<CaptureFunc> capture_func_;
    boost::function<StrErrorFunc> strerror_func_;
    boost::function<TouchFunc> touch_down_func_;
    boost::function<TouchFunc> touch_move_func_;
    boost::function<TouchUpFunc> touch_up_func_;
    boost::function<KeyFunc> key_func_;
    boost::function<InputTextFunc> input_text_func_;
};

MAA_CTRL_UNIT_NS_END

#endif
