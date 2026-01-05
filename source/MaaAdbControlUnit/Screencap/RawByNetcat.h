#pragma once

#include "Base/UnitBase.h"

#include "MaaUtils/IOStream/SockIOStream.h"
#include "ScreencapHelper.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawByNetcat : public ScreencapBase
{
public:
    virtual ~ScreencapRawByNetcat() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    std::optional<std::string> request_netcat_address();

    ProcessArgvGenerator screencap_raw_by_netcat_argv_;
    ProcessArgvGenerator netcat_address_argv_;

    std::string netcat_address_;

    std::shared_ptr<ServerSockIOFactory> io_factory_ = nullptr;
    std::shared_ptr<SockIOStream> sock_io_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
