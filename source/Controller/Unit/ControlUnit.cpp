#include "ControlUnit.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/StringMisc.hpp"

MAA_CTRL_UNIT_NS_BEGIN

void UnitHelper::set_io(std::shared_ptr<PlatformIO> io_ptr)
{
    io_ptr_ = std::move(io_ptr);
}

void UnitHelper::set_replacement(Argv::replacement argv_replace)
{
    argv_replace_ = std::move(argv_replace);
}

void UnitHelper::merge_replacement(Argv::replacement argv_replace, bool _override)
{
    if (_override) {
        argv_replace.merge(argv_replace_);
        argv_replace_ = std::move(argv_replace);
    }
    else {
        argv_replace_.merge(argv_replace);
    }
}

bool UnitHelper::parse_argv(const std::string& key, const json::value& config, Argv& argv)
{
    auto opt = config.find<json::value>(key);
    if (!opt) {
        LogError << "Cannot find key" << VAR(key);
        return false;
    }

    if (!argv.parse(*opt)) {
        LogError << "Parse config failed:" << VAR(key);
        return false;
    }

    return true;
}

std::optional<std::string> UnitHelper::command(Argv::value cmd, bool recv_by_socket, int64_t timeout)
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    LogInfo << cmd;

    auto start_time = std::chrono::steady_clock::now();

    std::string pipe_data;
    std::string sock_data;
    int ret = io_ptr_->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout);

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    LogInfo << VAR(cmd) << VAR(ret) << VAR(pipe_data.size()) << VAR(sock_data.size()) << VAR(duration);
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        LogTrace << Logger::separator::newline << "stdout output:" << pipe_data;
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        LogTrace << Logger::separator::newline << "socket output:" << sock_data;
    }

    if (ret != 0) {
        LogError << "call_command failed" << VAR(cmd) << VAR(ret);
        return std::nullopt;
    }

    return recv_by_socket ? sock_data : pipe_data;
}

bool Connection::parse(const json::value& config)
{
    return parse_argv("Connect", config, connect_argv_) && parse_argv("KillServer", config, kill_server_argv_);
}

bool Connection::connect()
{
    LogFunc;

    auto cmd_ret = command(connect_argv_.gen(argv_replace_), false, 60LL * 1000);

    if (!cmd_ret) {
        return false;
    }

    if (cmd_ret->find("error") != std::string::npos || cmd_ret->find("cannot") != std::string::npos) {
        return false;
    }

    return true;
}

bool Connection::kill_server()
{
    LogFunc;

    return command(kill_server_argv_.gen(argv_replace_), false, 60LL * 1000).has_value();
}

bool DeviceInfo::parse(const json::value& config)
{
    return parse_argv("Uuid", config, uuid_argv_) && parse_argv("Resolution", config, resolution_argv_) &&
           parse_argv("Orientation", config, orientation_argv_);
}

std::optional<std::string> DeviceInfo::uuid()
{
    LogFunc;

    auto cmd_ret = command(uuid_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return std::nullopt;
    }

    auto uuid_str = cmd_ret.value();
    std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });
    return uuid_str;
}

std::optional<DeviceInfo::Resolution> DeviceInfo::resolution()
{
    LogFunc;

    auto cmd_ret = command(resolution_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return std::nullopt;
    }

    std::istringstream iss(cmd_ret.value());
    int s1, s2;
    iss >> s1 >> s2;

    Resolution res;
    res.width = std::max(s1, s2);
    res.height = std::min(s1, s2);

    return res;
}

std::optional<int> DeviceInfo::orientation()
{
    LogFunc;

    auto cmd_ret = command(orientation_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return std::nullopt;
    }

    const auto& s = cmd_ret.value();

    if (s.empty()) {
        return std::nullopt;
    }

    auto ori = s.front() - '0';

    if (!(ori >= 0 && ori <= 3)) {
        return std::nullopt;
    }

    return ori;
}

bool Activity::parse(const json::value& config)
{
    return parse_argv("StartApp", config, start_app_argv_) && parse_argv("StopApp", config, stop_app_argv_);
}

bool Activity::start(const std::string& intent)
{
    LogFunc;

    merge_replacement({ { "{INTENT}", intent } });
    auto cmd_ret = command(start_app_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool Activity::stop(const std::string& intent)
{
    LogFunc;

    merge_replacement({ { "{INTENT}", intent } });
    auto cmd_ret = command(stop_app_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapInput::parse(const json::value& config)
{
    return parse_argv("Click", config, click_argv_) && parse_argv("Swipe", config, swipe_argv_) &&
           parse_argv("PressKey", config, press_key_argv_);
}

bool TapInput::click(int x, int y)
{
    LogFunc;

    merge_replacement({ { "{X}", std::to_string(x) }, { "{Y}", std::to_string(y) } });
    auto cmd_ret = command(click_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogFunc;

    merge_replacement({ { "{X1}", std::to_string(x1) },
                        { "{Y1}", std::to_string(y1) },
                        { "{X2}", std::to_string(x2) },
                        { "{Y2}", std::to_string(y2) },
                        { "{DURATION}", std::to_string(duration) } });
    auto cmd_ret = command(swipe_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapInput::press_key(int key)
{
    LogFunc;

    merge_replacement({ { "{KEY}", std::to_string(key) } });
    auto cmd_ret = command(press_key_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool Screencap::parse(const json::value& config)
{
    return parse_argv("ScreencapRawByNetcat", config, screencap_raw_by_netcat_argv_) &&
           parse_argv("NetcatAddress", config, netcat_address_argv_) &&
           parse_argv("ScreencapRawWithGzip", config, screencap_raw_with_gzip_argv_) &&
           parse_argv("ScreencapEncode", config, screencap_encode_argv_) &&
           parse_argv("ScreencapEncodeToFile", config, screencap_encode_to_file_argv_) &&
           parse_argv("PullFile", config, pull_file_argv_);
}

bool Screencap::init(int w, int h)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    width = w;
    height = h;

    auto addr = netcat_address();
    if (!addr.has_value()) {
        return false;
    }
    address = addr.value();

    auto prt = io_ptr_->create_socket(address);
    if (!prt.has_value()) {
        return false;
    }
    port = prt.value();

    return true;
}

void Screencap::deinit()
{
    if (!io_ptr_) {
        return;
    }

    if (port) {
        io_ptr_->close_socket();
    }

    width = 0;
    height = 0;
    address = "";
    port = 0;
    end_of_line = Screencap::EndOfLine::UnknownYet;
    method = Screencap::Method::UnknownYet;
}

std::optional<cv::Mat> Screencap::decode(const std::string& buffer)
{
    if (buffer.size() < 8) {
        return std::nullopt;
    }

    auto data = static_cast<const uint8_t*>(static_cast<const void*>(buffer.c_str()));
    uint32_t im_width, im_height;
    memcpy(&im_width, data, 4);
    memcpy(&im_height, data + 4, 4);

    if (int(im_width) != width || int(im_height) != height) {
        LogError << "screencap size image" << im_width << im_height << "don't match" << width << height;
        return std::nullopt;
    }

    auto size = 4ull * im_width * im_height;

    if (buffer.size() < size) {
        return std::nullopt;
    }

    auto hdrSize = buffer.size() - size;
    auto im_data = data + hdrSize;

    cv::Mat temp(height, width, CV_8UC4, const_cast<uint8_t*>(im_data));
    if (temp.empty()) {
        return std::nullopt;
    }

    const auto& br = *(temp.end<cv::Vec4b>() - 1);
    if (br[3] != 255) { // only check alpha
        return std::nullopt;
    }
    cv::cvtColor(temp, temp, cv::COLOR_RGBA2BGR);
    return temp.clone(); // temp只是引用data, 需要clone确保数据拥有所有权
}

std::optional<cv::Mat> Screencap::screencap_raw_by_netcat()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    merge_replacement({ { "{NC_ADDRESS}", address }, { "{NC_PORT}", std::to_string(port) } });
    auto cmd_ret = command(screencap_raw_by_netcat_argv_.gen(argv_replace_), true);

    if (!cmd_ret.has_value()) {
        return std::nullopt;
    }

    return decode(cmd_ret.value());
}

std::optional<std::string> Screencap::netcat_address()
{
    LogFunc;

    auto cmd_ret = command(netcat_address_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
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

bool InvokeApp::parse(const json::value& config)
{
    return parse_argv("Abilist", config, abilist_argv_) && parse_argv("PushBin", config, push_bin_argv_) &&
           parse_argv("ChmodBin", config, chmod_bin_argv_) && parse_argv("InvokeBin", config, invoke_bin_argv_) &&
           parse_argv("InvokeApp", config, invoke_app_argv_);
}

std::optional<std::vector<std::string>> InvokeApp::abilist()
{
    LogFunc;

    auto cmd_ret = command(abilist_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return std::nullopt;
    }

    auto abils = cmd_ret.value();
    while (abils.back() == '\n' || abils.back() == '\r') {
        abils.pop_back();
    }
    std::vector<std::string> res;

    while (abils.length() > 0) {
        auto pos = abils.find(',');
        res.push_back(abils.substr(0, pos));
        abils = abils.substr(pos + 1);
        if (pos == std::string::npos) {
            break;
        }
    }

    return res;
}

MAA_CTRL_UNIT_NS_END
