#include "RawByNetcat.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapRawByNetcat::parse(const json::value& config)
{
    return parse_argv("ScreencapRawByNetcat", config, screencap_raw_by_netcat_argv_) &&
           parse_argv("NetcatAddress", config, netcat_address_argv_);
}

bool ScreencapRawByNetcat::init(int swidth, int sheight)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    auto addr = request_netcat_address();
    if (!addr) {
        return false;
    }
    netcat_address_ = addr.value();

    auto serial_host = argv_replace_["{ADB_SERIAL}"];
    auto shp = serial_host.find(':');
    std::string local = "127.0.0.1";
    if (shp != std::string::npos) {
        local = serial_host.substr(0, shp);
    }

    auto prt = io_ptr_->create_socket(local);
    if (!prt) {
        return false;
    }
    netcat_port_ = prt.value();

    return set_wh(swidth, sheight);
}

void ScreencapRawByNetcat::deinit()
{
    if (netcat_port_ && io_ptr_) {
        io_ptr_->close_socket();
    }

    netcat_address_.clear();
    netcat_port_ = 0;
}

std::optional<cv::Mat> ScreencapRawByNetcat::screencap()
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    merge_replacement({ { "{NETCAT_ADDRESS}", netcat_address_ }, { "{NETCAT_PORT}", std::to_string(netcat_port_) } });
    constexpr int kTimeout = 2000; // netcat 能用的时候一般都很快，但连不上的时候会一直卡着，所以超时设短一点
    auto cmd_ret = command(screencap_raw_by_netcat_argv_.gen(argv_replace_), true, kTimeout);

    if (!cmd_ret) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(
        cmd_ret.value(), std::bind(&ScreencapHelper::decode_raw, &screencap_helper_, std::placeholders::_1));
}

std::optional<std::string> ScreencapRawByNetcat::request_netcat_address()
{
    LogFunc;

    auto cmd_ret = command(netcat_address_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    auto ip = cmd_ret.value();
    auto idx = ip.find(' ');

    if (idx != std::string::npos) {
        return ip.substr(0, idx);
    }
    else {
        return std::nullopt;
    }
}

MAA_CTRL_UNIT_NS_END
