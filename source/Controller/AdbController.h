#pragma once
#include "ControllerMgr.h"

#include "Platform/PlatformIO.h"
#include "Resource/AdbConfig.h"

MAA_CTRL_NS_BEGIN

class AdbController : public ControllerMgr
{
public:
    AdbController(const std::string& adb_path, const std::string& address, MaaControllerCallback callback,
                  void* callback_arg);
    virtual ~AdbController() override;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;

protected:
    std::shared_ptr<MAA_RES_NS::AdbConfig> adb_cfg() const;
    std::optional<std::string> command(const std::string& cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::string adb_path_;
    std::string address_;

protected:
    std::shared_ptr<PlatformIO> platform_io_ = nullptr;
    bool support_socket_ = false;

    // struct Command
    //{
    //     std::string connect;
    //     std::string kill_server;

    //    std::string start_app;
    //    std::string stop_app;

    //    std::string click;
    //    std::string swipe;
    //    std::string press_key;

    //    std::string screencap;
    //    std::optional<std::string> pull_file;
    //} commands_;

    struct ScreencapProperty
    {
        enum class ScreencapEndOfLine
        {
            UnknownYet,
            CRLF,
            LF,
            CR
        } screencap_end_of_line = ScreencapEndOfLine::UnknownYet;

        enum class ScreencapMethod
        {
            UnknownYet,
            RawByNetcat,
            RawWithGzip,
            Encode,
            EncodeToFileAndPull,
        } screencap_method = ScreencapMethod::UnknownYet;
    } screencap_property_;
};

MAA_CTRL_NS_END
