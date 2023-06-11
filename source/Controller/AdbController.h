#pragma once
#include "ControllerMgr.h"

#include "Platform/PlatformIO.h"
#include "Unit/Unit.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class AdbController : public ControllerMgr
{
public:
    AdbController(const std::string& adb_path, const std::string& address, const json::value& config,
                  MaaControllerCallback callback, void* callback_arg);
    virtual ~AdbController() override;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;

private:
    std::string adb_path_;
    std::string address_;

    std::shared_ptr<MAA_CTRL_UNIT_NS::Connection> connection_unit_ = nullptr;

private:
    std::shared_ptr<PlatformIO> platform_io_ = nullptr;
    bool support_socket_ = false;

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
