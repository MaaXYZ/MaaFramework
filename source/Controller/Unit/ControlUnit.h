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

class UnitBase
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

class Connection : public UnitBase
{
public:
    bool parse(const json::value& config);

    bool connect();
    bool kill_server();

private:
    Argv connect_argv_;
    Argv kill_server_argv_;
};

class DeviceInfo : public UnitBase
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

class Activity : public UnitBase
{
public:
    bool parse(const json::value& config);

    bool start(const std::string& intent);
    bool stop(const std::string& intent);

private:
    Argv start_app_argv_;
    Argv stop_app_argv_;
};

class TapInput : public UnitBase
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

class ScreencapBase : public UnitBase
{
public:
    void set_wh(int w, int h);
    // int get_w() const { return width_; }
    // int get_h() const { return height_; }

    std::optional<cv::Mat> process_data(std::string& buffer,
                                        std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder);
    std::optional<cv::Mat> decode_raw(const std::string& buffer);
    std::optional<cv::Mat> decode_gzip(const std::string& buffer);
    std::optional<cv::Mat> decode_png(const std::string& buffer);
    static bool clean_cr(std::string& buffer);

private:
    int width_ = 0;
    int height_ = 0;

    enum class EndOfLine
    {
        UnknownYet,
        CRLF,
        LF,
        CR
    } end_of_line_ = EndOfLine::UnknownYet;
};

class ScreencapRawByNetcat : public ScreencapBase
{
public:
    bool parse(const json::value& config);

    bool init(int w, int h);
    void deinit();

    std::optional<cv::Mat> screencap();

private:
    std::optional<std::string> request_netcat_address();

    Argv screencap_raw_by_netcat_argv_;
    Argv netcat_address_argv_;

    std::string netcat_address_;
    uint16_t netcat_port_ = 0;
};

class ScreencapRawWithGzip : public ScreencapBase
{
public:
    bool parse(const json::value& config);

    bool init(int w, int h);
    void deinit() {}

    std::optional<cv::Mat> screencap();

private:
    Argv screencap_raw_with_gzip_argv_;
};

class ScreencapEncode : public ScreencapBase
{
public:
    bool parse(const json::value& config);

    bool init(int w, int h);
    void deinit() {}

    std::optional<cv::Mat> screencap();

private:
    Argv screencap_encode_argv_;
};

class ScreencapEncodeToFileAndPull : public ScreencapBase
{
public:
    bool parse(const json::value& config);

    bool init(int w, int h, const std::string& force_temp = "");
    void deinit() {}

    std::optional<cv::Mat> screencap();

#ifdef MAA_DEBUG
    const std::string& get_tempname() const { return tempname_; }
#endif

private:
    Argv screencap_encode_to_file_argv_;
    Argv pull_file_argv_;

    std::string tempname_;
};

class Screencap : public UnitBase
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

private:
    bool speed_test();

    ScreencapRawByNetcat screencap_raw_by_netcat_uint_;
    ScreencapRawWithGzip screencap_raw_with_gzip_unit_;
    ScreencapEncode screencap_encode_unit_;
    ScreencapEncodeToFileAndPull screencap_encode_to_file_unit_;

    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, Screencap::Method m);

class InvokeApp : public UnitBase
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

class MinitouchInput : public UnitBase
{
public:
    struct Step
    {
        int x, y;
        int delay;
    };

    MinitouchInput() : invoke_app_(new InvokeApp) {}

    bool parse(const json::value& config);

    bool init(int swidth, int sheight, std::function<std::string(const std::string&)> path_of_bin,
              const std::string& force_temp = "");

    bool click(int x, int y);
    bool swipe(const std::vector<Step>& steps);
    bool press_key(int key);

private:
    std::shared_ptr<InvokeApp> invoke_app_;
    std::shared_ptr<IOHandler> shell_handler_;
    int width, height;
    double xscale, yscale, press;
};

MAA_CTRL_UNIT_NS_END
