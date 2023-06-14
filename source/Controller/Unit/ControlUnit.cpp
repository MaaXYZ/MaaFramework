#define _CRT_SECURE_NO_WARNINGS

#include "ControlUnit.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/StringMisc.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4068)
#endif
#include <gzip/decompress.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

MAA_CTRL_UNIT_NS_BEGIN

std::string temp_name()
{
    char p[L_tmpnam] = { 0 };
#ifdef _WIN32
    tmpnam(p);
    auto pos = std::string(p).find_last_of("\\/");
    LogInfo << p << pos << (p + pos + 1);
    return p + pos + 1;
#else
    auto _ret __attribute__((unused)) = tmpnam(p);
    return p + sizeof(P_tmpdir);
#endif
}

std::filesystem::path temp_path(const std::string& name)
{
#ifdef _WIN32
    return std::filesystem::temp_directory_path() / name;
#else
    // 虽然可以就用上面那个, 但是不确定这是否是同一个, 保险起见还是用现有的marcro
    return std::filesystem::path(P_tmpdir) / name;
#endif
}

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

std::optional<std::string> DeviceInfo::request_uuid()
{
    LogFunc;

    auto cmd_ret = command(uuid_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    auto& uuid_str = cmd_ret.value();
    std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });

    uuid_ = std::move(uuid_str);
    return uuid_;
}

std::optional<DeviceInfo::Resolution> DeviceInfo::request_resolution()
{
    LogFunc;

    auto cmd_ret = command(resolution_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    std::istringstream iss(cmd_ret.value());
    int s1, s2;
    iss >> s1 >> s2;

    Resolution res;
    res.width = std::max(s1, s2);
    res.height = std::min(s1, s2);

    resolution_ = std::move(res);
    return resolution_;
}

std::optional<int> DeviceInfo::request_orientation()
{
    LogFunc;

    auto cmd_ret = command(orientation_argv_.gen(argv_replace_));

    if (!cmd_ret) {
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

    orientation_ = ori;
    return orientation_;
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

    return cmd_ret.has_value();
}

bool Activity::stop(const std::string& intent)
{
    LogFunc;

    merge_replacement({ { "{INTENT}", intent } });
    auto cmd_ret = command(stop_app_argv_.gen(argv_replace_));

    return cmd_ret.has_value();
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

bool Screencap::init(int w, int h, const std::string& force_temp)
{
    LogFunc;

    tempname_ = force_temp.empty() ? temp_name() : force_temp;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    width_ = w;
    height_ = h;

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

    return speed_test();
}

void Screencap::deinit()
{
    if (netcat_port_ && io_ptr_) {
        io_ptr_->close_socket();
    }

    width_ = 0;
    height_ = 0;
    netcat_address_.clear();
    netcat_port_ = 0;
    end_of_line_ = EndOfLine::UnknownYet;
    method_ = Method::UnknownYet;
}

std::optional<cv::Mat> Screencap::screencap()
{
    switch (method_) {
    case Method::UnknownYet:
        LogError << "Unknown screencap method";
        return std::nullopt;
    case Method::RawByNetcat:
        return screencap_raw_by_netcat();
    case Method::RawWithGzip:
        return screencap_raw_with_gzip();
    case Method::Encode:
        return screencap_encode();
    case Method::EncodeToFileAndPull:
        return screencap_encode_to_file();
    }
    return std::nullopt;
}

std::optional<cv::Mat> Screencap::process_data(std::string& buffer,
                                               std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder)
{
    bool tried_clean = false;

    if (end_of_line_ == EndOfLine::CRLF) {
        tried_clean = true;
        if (!clean_cr(buffer)) {
            LogInfo << "end_of_line is set to CRLF but no `\\r\\n` found, set it to LF";
            end_of_line_ = EndOfLine::LF;
        }
    }

    auto res = decoder(buffer);

    if (res) {
        if (end_of_line_ == EndOfLine::UnknownYet) {
            LogInfo << "end_of_line is LF";
            end_of_line_ = EndOfLine::LF;
        }
        return res;
    }

    LogInfo << "data is not empty, but image is empty";
    if (tried_clean) {
        LogError << "skip retry decoding and decode failed!";
        return std::nullopt;
    }

    LogInfo << "try to cvt lf";
    if (!clean_cr(buffer)) {
        LogError << "no `\\r\\n` found, skip retry decode";
        return std::nullopt;
    }

    res = decoder(buffer);

    if (!res) {
        LogError << "convert lf and retry decode failed!";
        return std::nullopt;
    }

    if (end_of_line_ == EndOfLine::UnknownYet) {
        LogInfo << "end_of_line is CRLF";
    }
    else {
        LogInfo << "end_of_line is changed to CRLF";
    }
    end_of_line_ = EndOfLine::CRLF;

    return res;
}

std::optional<cv::Mat> Screencap::decode_raw(const std::string& buffer)
{
    if (buffer.size() < 8) {
        return std::nullopt;
    }

    auto data = static_cast<const uint8_t*>(static_cast<const void*>(buffer.c_str()));
    uint32_t im_width, im_height;
    memcpy(&im_width, data, 4);
    memcpy(&im_height, data + 4, 4);

    if (int(im_width) != width_ || int(im_height) != height_) {
        LogError << "screencap size image" << VAR(im_width) << VAR(im_height) << "don't match" << VAR(width_)
                 << VAR(height_);
        return std::nullopt;
    }

    auto size = 4ull * im_width * im_height;

    if (buffer.size() < size) {
        return std::nullopt;
    }

    auto hdrSize = buffer.size() - size;
    auto im_data = data + hdrSize;

    cv::Mat temp(height_, width_, CV_8UC4, const_cast<uint8_t*>(im_data));
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

std::optional<cv::Mat> Screencap::decode_gzip(const std::string& buffer)
{
    auto buf = gzip::decompress(buffer.c_str(), buffer.size());
    return decode_raw(buf);
}

std::optional<cv::Mat> Screencap::decode_png(const std::string& buffer)
{
    cv::Mat temp = cv::imdecode({ buffer.data(), int(buffer.size()) }, cv::IMREAD_COLOR);
    if (temp.empty()) {
        return std::nullopt;
    }

    return temp.clone();
}

bool Screencap::clean_cr(std::string& buffer)
{
    if (buffer.size() < 2) {
        return false;
    }

    auto check = [](std::string::iterator it) { return *it == '\r' && *(it + 1) == '\n'; };

    auto scan = buffer.end();
    for (auto it = buffer.begin(); it != buffer.end() - 1; ++it) {
        if (check(it)) {
            scan = it;
            break;
        }
    }
    if (scan == buffer.end()) {
        return false;
    }

    // TODO: 应该可以优化为若干次copy+find, 效率应该会好一点, 但是没必要
    auto last = buffer.end() - 1;
    auto ptr = scan;
    while (++scan != last) {
        if (!check(scan)) {
            *ptr = *scan;
            ++ptr;
        }
    }
    *ptr = *last;
    ++ptr;
    buffer.erase(ptr, buffer.end());
    return true;
}

std::optional<cv::Mat> Screencap::screencap_raw_by_netcat()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    merge_replacement({ { "{NETCAT_ADDRESS}", netcat_address_ }, { "{NETCAT_PORT}", std::to_string(netcat_port_) } });
    auto cmd_ret = command(screencap_raw_by_netcat_argv_.gen(argv_replace_), true);

    if (!cmd_ret) {
        return std::nullopt;
    }

    return process_data(cmd_ret.value(), std::bind(&Screencap::decode_raw, this, std::placeholders::_1));
}

std::optional<cv::Mat> Screencap::screencap_raw_with_gzip()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto cmd_ret = command(screencap_raw_with_gzip_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return process_data(cmd_ret.value(), std::bind(&Screencap::decode_gzip, this, std::placeholders::_1));
}

std::optional<cv::Mat> Screencap::screencap_encode()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto cmd_ret = command(screencap_encode_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return process_data(cmd_ret.value(), std::bind(&Screencap::decode_png, this, std::placeholders::_1));
}

std::optional<cv::Mat> Screencap::screencap_encode_to_file()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto dst_path = temp_path(tempname_);

    merge_replacement({ { "{TEMP_FILE}", tempname_ }, { "{DST_PATH}", path_to_crt_string(dst_path) } });
    auto cmd_ret = command(screencap_encode_to_file_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    cmd_ret = command(pull_file_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    std::ifstream f(dst_path, std::ios_base::in | std::ios_base::binary);
    if (!f.is_open()) {
        return std::nullopt;
    }

    f.seekg(0, std::ios_base::end);
    size_t l = f.tellg();
    std::string buf(l, '\0');
    f.seekg(0, std::ios_base::beg);
    f.read(buf.data(), l);
    f.close();

    return process_data(buf, std::bind(&Screencap::decode_png, this, std::placeholders::_1));
}

std::optional<std::string> Screencap::request_netcat_address()
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

bool Screencap::speed_test()
{
    LogFunc;

    method_ = Method::UnknownYet;
    std::chrono::nanoseconds cost(INT64_MAX);

    auto check = [this, &cost](Method method, std::chrono::steady_clock::time_point start) {
        auto duration = std::chrono::steady_clock::now() - start;
        if (duration < cost) {
            method_ = method;
            cost = duration;
        }
        LogInfo << VAR(method) << VAR(duration);
    };

    {
        end_of_line_ = EndOfLine::UnknownYet;
        auto now = std::chrono::steady_clock::now();
        if (auto img = screencap_raw_by_netcat()) {
            check(Method::RawByNetcat, now);
        }
    }

    {
        end_of_line_ = EndOfLine::UnknownYet;
        auto now = std::chrono::steady_clock::now();
        if (auto img = screencap_raw_with_gzip()) {
            check(Method::RawWithGzip, now);
        }
    }

    {
        end_of_line_ = EndOfLine::UnknownYet;
        auto now = std::chrono::steady_clock::now();
        if (auto img = screencap_encode()) {
            check(Method::Encode, now);
        }
    }

    {
        end_of_line_ = EndOfLine::UnknownYet;
        auto now = std::chrono::steady_clock::now();
        if (auto img = screencap_encode_to_file()) {
            check(Method::EncodeToFileAndPull, now);
        }
    }

    end_of_line_ = EndOfLine::UnknownYet;

    if (method_ == Method::UnknownYet) {
        LogError << "cannot find any method to screencap!";
        return false;
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(cost);
    LogInfo << "The fastest method is " << method_ << VAR(ms);
    return true;
}

std::ostream& operator<<(std::ostream& os, Screencap::Method m)
{
    switch (m) {
    case Screencap::Method::UnknownYet:
        os << "UnknownYet";
        break;
    case Screencap::Method::RawByNetcat:
        os << "RawByNetcat";
        break;
    case Screencap::Method::RawWithGzip:
        os << "RawWithGzip";
        break;
    case Screencap::Method::Encode:
        os << "Encode";
        break;
    case Screencap::Method::EncodeToFileAndPull:
        os << "EncodeToFileAndPull";
        break;
    }
    return os;
}

bool InvokeApp::parse(const json::value& config)
{
    return parse_argv("Abilist", config, abilist_argv_) && parse_argv("PushBin", config, push_bin_argv_) &&
           parse_argv("ChmodBin", config, chmod_bin_argv_) && parse_argv("InvokeBin", config, invoke_bin_argv_) &&
           parse_argv("InvokeApp", config, invoke_app_argv_);
}

bool InvokeApp::init(const std::string& force_temp)
{
    LogFunc;

    tempname_ = force_temp.empty() ? temp_name() : force_temp;
    return true;
}

std::optional<std::vector<std::string>> InvokeApp::abilist()
{
    LogFunc;

    auto cmd_ret = command(abilist_argv_.gen(argv_replace_));

    if (!cmd_ret) {
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

bool InvokeApp::push(const std::string& path)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    merge_replacement({ { "{BIN_PATH}", path }, { "{BIN_WORKING_FILE}", tempname_ } });
    auto cmd_ret = command(push_bin_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return false;
    }

    return true;
}

bool InvokeApp::chmod()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ } });
    auto cmd_ret = command(chmod_bin_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return false;
    }

    return true;
}

std::shared_ptr<IOHandler> InvokeApp::invoke_bin(const std::string& extra)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return nullptr;
    }

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ }, { "{BIN_EXTRA_PARAMS}", extra } });
    LogInfo << invoke_bin_argv_.gen(argv_replace_);
    auto cmd_ret = io_ptr_->interactive_shell(invoke_bin_argv_.gen(argv_replace_));

    return cmd_ret;
}

std::shared_ptr<IOHandler> InvokeApp::invoke_app(const std::string& package)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return nullptr;
    }

    merge_replacement({ { "{APP_WORKING_FILE}", tempname_ }, { "{PACKAGE_NAME}", package } });
    LogInfo << invoke_app_argv_.gen(argv_replace_);
    auto cmd_ret = io_ptr_->interactive_shell(invoke_app_argv_.gen(argv_replace_));

    return cmd_ret;
}

MAA_CTRL_UNIT_NS_END
