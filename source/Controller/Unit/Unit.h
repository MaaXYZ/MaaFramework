#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include "Controller/Platform/PlatformIO.h"

#include "Utils/ArgvWrapper.hpp"

#define MAA_CTRL_UNIT_NS MAA_CTRL_NS::Unit
#define MAA_CTRL_UNIT_NS_BEGIN \
    namespace MAA_CTRL_UNIT_NS \
    {
#define MAA_CTRL_UNIT_NS_END }

MAA_CTRL_UNIT_NS_BEGIN

class UnitHelper : NonCopyable
{
public:
    using Argv = ArgvWrapper<std::vector<std::string>>;

public:
    UnitHelper(std::shared_ptr<PlatformIO> io_ptr);

    void set_replacement(Argv::replacement argv_replace);

protected:
    static bool parse_argv(const std::string& key, const json::value& config, /*out*/ Argv& argv);

    std::optional<std::string> command(Argv::value cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::shared_ptr<PlatformIO> io_ptr_;
    Argv::replacement argv_replace_;
};

class Connection : public UnitHelper
{
public:
    using UnitHelper::UnitHelper;

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
    using UnitHelper::UnitHelper;

public:
    bool parse(const json::value& config);

    bool uuid(std::string& uuid);
    bool resolution(int& width, int& height); // width is always greater than height
    bool orientation(int& ori);               // 0 ~ 3

private:
    Argv uuid_argv_;
    Argv resolution_argv_;
    Argv orientation_argv_;
};

class Activity : public UnitHelper
{
public:
    using UnitHelper::UnitHelper;

private:
    Argv start_app_argv_;
    Argv stop_app_argv_;
};

class TapInput : public UnitHelper
{
public:
    using UnitHelper::UnitHelper;

private:
    Argv click_argv_;
    Argv swipe_argv_;
    Argv press_key_argv_;
};

class ScreenShot : public UnitHelper
{
public:
    using UnitHelper::UnitHelper;

private:
    Argv screencap_raw_by_netcat_argv_;
    Argv netcat_address_argv_;
    Argv screencap_raw_with_gzip_argv_;
    Argv screencap_encode_argv_;
    Argv screencap_encode_to_file_argv_;
    Argv pull_file_argv_;
};

class InvokeApp : public UnitHelper
{
public:
    using UnitHelper::UnitHelper;

private:
    Argv abilist_argv_;
    Argv push_bin_argv_;
    Argv chmod_bin_argv_;
    Argv invoke_bin_argv_;
    Argv invoke_app_argv_;
};

MAA_CTRL_UNIT_NS_END
