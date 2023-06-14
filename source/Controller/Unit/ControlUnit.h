#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include "Controller/Platform/PlatformIO.h"
#include "Utils/ArgvWrapper.hpp"
#include "Utils/NoWarningCVMat.h"

#define MAA_CTRL_UNIT_NS MAA_CTRL_NS::Unit
#define MAA_CTRL_UNIT_NS_BEGIN \
    namespace MAA_CTRL_UNIT_NS \
    {
#define MAA_CTRL_UNIT_NS_END }

MAA_CTRL_UNIT_NS_BEGIN

class UnitHelper
{
public:
    using Argv = ArgvWrapper<std::vector<std::string>>;

public:
    void set_io(std::shared_ptr<PlatformIO> io_ptr);
    void set_replacement(Argv::replacement argv_replace);
    void merge_replacement(Argv::replacement argv_replace, bool _override = true);

protected:
    static bool parse_argv(const std::string& key, const json::value& config, /*out*/ Argv& argv);

    std::optional<std::string> command(Argv::value cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::shared_ptr<PlatformIO> io_ptr_ = nullptr;
    Argv::replacement argv_replace_;
};

class Connection : public UnitHelper
{
public:
    bool parse(const json::value& config);

    bool connect();
    bool kill_server();

private:
    Argv connect_argv_;
    Argv kill_server_argv_;
};

class DeviceInfo : public UnitHelper
{
public:
    struct Resolution
    {
        int width = 0;
        int height = 0;
    };

    bool parse(const json::value& config);

    std::optional<std::string> request_uuid();
    std::optional<Resolution> request_resolution();
    std::optional<int> request_orientation();

public:
    const std::string& get_uuid() const { return uuid_; }
    const Resolution& get_resolution() const { return resolution_; }
    int get_orientation() const { return orientation_; }

private:
    Argv uuid_argv_;
    Argv resolution_argv_;
    Argv orientation_argv_;

private:
    std::string uuid_;
    Resolution resolution_;
    int orientation_;
};

class Activity : public UnitHelper
{
public:
    bool parse(const json::value& config);

    bool start(const std::string& intent);
    bool stop(const std::string& intent);

private:
    Argv start_app_argv_;
    Argv stop_app_argv_;
};

class TapInput : public UnitHelper
{
public:
    bool parse(const json::value& config);

    bool click(int x, int y);
    bool swipe(int x1, int y1, int x2, int y2, int duration);
    bool press_key(int key);

private:
    Argv click_argv_;
    Argv swipe_argv_;
    Argv press_key_argv_;
};

class Screencap : public UnitHelper
{
public:
    enum class Method
    {
        UnknownYet,
        RawByNetcat,
        RawWithGzip,
        Encode,
        EncodeToFileAndPull,
    };

public:
    bool parse(const json::value& config);

    bool init(int w, int h, const std::string& force_temp = "");
    void deinit();

    std::optional<cv::Mat> screencap();

#ifdef MAA_DEBUG
    const std::string& get_tempname() const { return tempname_; }
#endif

private:
    std::optional<cv::Mat> screencap_raw_by_netcat();
    std::optional<cv::Mat> screencap_raw_with_gzip();
    std::optional<cv::Mat> screencap_encode();
    std::optional<cv::Mat> screencap_encode_to_file();

private:
    bool speed_test();
    std::optional<cv::Mat> process_data(std::string& buffer,
                                        std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder);
    std::optional<cv::Mat> decode_raw(const std::string& buffer);
    std::optional<cv::Mat> decode_gzip(const std::string& buffer);
    std::optional<cv::Mat> decode_png(const std::string& buffer);
    std::optional<std::string> request_netcat_address();
    bool clean_cr(std::string& buffer);

private:
    Argv screencap_raw_by_netcat_argv_;
    Argv netcat_address_argv_;
    Argv screencap_raw_with_gzip_argv_;
    Argv screencap_encode_argv_;
    Argv screencap_encode_to_file_argv_;
    Argv pull_file_argv_;

private:
    std::string tempname_;

    int width_ = 0;
    int height_ = 0;
    std::string netcat_address_;
    uint16_t netcat_port_ = 0;

    enum class EndOfLine
    {
        UnknownYet,
        CRLF,
        LF,
        CR
    } end_of_line_ = EndOfLine::UnknownYet;

    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, Screencap::Method m);

class InvokeApp : public UnitHelper
{
public:
    bool parse(const json::value& config);

    bool init(const std::string& force_temp = "");

    std::optional<std::vector<std::string>> abilist();
    bool push(const std::string& path);
    bool chmod();
    std::shared_ptr<IOHandler> invoke_bin(const std::string& extra);
    std::shared_ptr<IOHandler> invoke_app(const std::string& package);

#ifdef MAA_DEBUG
    std::string get_tempname() const { return tempname_; }
#endif
    
private:
    Argv abilist_argv_;
    Argv push_bin_argv_;
    Argv chmod_bin_argv_;
    Argv invoke_bin_argv_;
    Argv invoke_app_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
