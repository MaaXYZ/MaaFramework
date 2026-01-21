#include "RawByNetcat.h"

#include "MaaUtils/IOStream/ChildPipeIOStream.h"
#include "MaaUtils/IOStream/SockIOStream.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapRawByNetcat::parse(const json::value& config)
{
    static const json::array kDefaultScreencapRawByNetcatArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "exec-out", "screencap | nc -w 3 {NETCAT_ADDRESS} {NETCAT_PORT}",
    };
    static const json::array kDefaultNetcatAddressArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "cat /proc/net/arp | grep : ",
    };

    return parse_command("ScreencapRawByNetcat", config, kDefaultScreencapRawByNetcatArgv, screencap_raw_by_netcat_argv_)
           && parse_command("NetcatAddress", config, kDefaultNetcatAddressArgv, netcat_address_argv_);
}

bool ScreencapRawByNetcat::init()
{
    LogFunc;

    auto addr_opt = request_netcat_address();
    if (!addr_opt) {
        return false;
    }
    netcat_address_ = std::move(*addr_opt);

    auto serial_host = argv_replace_["{ADB_SERIAL}"];
    auto shp = serial_host.find(':');
    std::string local = "127.0.0.1";
    if (shp != std::string::npos) {
        local = serial_host.substr(0, shp);
    }

    io_factory_ = std::make_shared<ServerSockIOFactory>(local, 0);

    return true;
}

std::optional<cv::Mat> ScreencapRawByNetcat::screencap()
{
    if (!io_factory_) {
        return std::nullopt;
    }

    auto port = io_factory_->port();
    merge_replacement({ { "{NETCAT_ADDRESS}", netcat_address_ }, { "{NETCAT_PORT}", std::to_string(port) } });

    auto argv_opt = screencap_raw_by_netcat_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto& argv = *argv_opt;

    ChildPipeIOStream child(argv.exec, argv.args);

    auto ios = io_factory_->accept();
    if (!ios) {
        LogError << "accept failed" << VAR(argv.exec) << VAR(argv.args);
        return std::nullopt;
    }

    using namespace std::chrono_literals;
    // netcat 能用的时候一般都很快，但连不上的时候会一直卡着，所以超时设短一点
    ios->expires_after(1s);
    std::string output = ios->read(1s);
    ios->release();

    if (!child.release()) {
        LogWarn << "child return error" << VAR(argv.exec) << VAR(argv.args);
    }

    return screencap_helper_.process_data(output, ScreencapHelper::decode_raw);
}

std::optional<std::string> ScreencapRawByNetcat::request_netcat_address()
{
    LogFunc;

    auto argv_opt = netcat_address_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    auto& ip = *output_opt;
    auto idx = ip.find(' ');

    if (idx == std::string::npos) {
        return std::nullopt;
    }

    return ip.substr(0, idx);
}

MAA_CTRL_UNIT_NS_END
