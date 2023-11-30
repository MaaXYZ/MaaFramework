#include "InvokeApp.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Time.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool InvokeApp::parse(const json::value& config)
{
    static const json::array kDefaultAbilistArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "getprop ro.product.cpu.abilist | tr -d '\n\r'",
    };
    static const json::array kDefaultSdkArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "getprop ro.build.version.sdk | tr -d '\n\r'",
    };
    static const json::array kDefaultPushBinArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "push", "{BIN_PATH}", "/data/local/tmp/{BIN_WORKING_FILE}",
    };
    static const json::array kDefaultChmodBinArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "chmod 700 \"/data/local/tmp/{BIN_WORKING_FILE}\"",
    };
    static const json::array kDefaultInvokeBinArgv = {
        "{ADB}",
        "-s",
        "{ADB_SERIAL}",
        "shell",
        "export LD_LIBRARY_PATH=/data/local/tmp/; \"/data/local/tmp/{BIN_WORKING_FILE}\" {BIN_EXTRA_PARAMS} 2>&1",
    };
    static const json::array kDefaultInvokeAppArgv = {
        "{ADB}",
        "-s",
        "{ADB_SERIAL}",
        "shell",
        "export CLASSPATH=\"/data/local/tmp/{APP_WORKING_FILE}\"; app_process /data/local/tmp {PACKAGE_NAME}",
    };

    return parse_argv("Abilist", config, kDefaultAbilistArgv, abilist_argv_) &&
           parse_argv("SDK", config, kDefaultSdkArgv, sdk_argv_) &&
           parse_argv("PushBin", config, kDefaultPushBinArgv, push_bin_argv_) &&
           parse_argv("ChmodBin", config, kDefaultChmodBinArgv, chmod_bin_argv_) &&
           parse_argv("InvokeBin", config, kDefaultInvokeBinArgv, invoke_bin_argv_) &&
           parse_argv("InvokeApp", config, kDefaultInvokeAppArgv, invoke_app_argv_);
}

bool InvokeApp::init(const std::string& force_temp)
{
    LogFunc;

    tempname_ = force_temp.empty() ? now_filestem() : force_temp;
    return true;
}

std::optional<std::vector<std::string>> InvokeApp::abilist()
{
    LogFunc;

    auto cmd_ret = command(abilist_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return string_split(*cmd_ret, ',');
}

std::optional<int> InvokeApp::sdk()
{
    LogFunc;

    auto cmd_ret = command(sdk_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return atoi(cmd_ret.value().c_str());
}

bool InvokeApp::push(const std::filesystem::path& path)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    std::string absolute_path = path_to_crt_string(std::filesystem::absolute(path));
    merge_replacement({ { "{BIN_PATH}", absolute_path }, { "{BIN_WORKING_FILE}", tempname_ } });
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

std::optional<std::string> InvokeApp::invoke_bin_stdout(const std::string& extra)
{
    LogFunc;

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ }, { "{BIN_EXTRA_PARAMS}", extra } });
    LogInfo << invoke_bin_argv_.gen(argv_replace_);
    return command(invoke_bin_argv_.gen(argv_replace_));
}

std::shared_ptr<IOHandler> InvokeApp::invoke_bin(const std::string& extra, bool wants_stderr)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return nullptr;
    }

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ }, { "{BIN_EXTRA_PARAMS}", extra } });
    LogInfo << invoke_bin_argv_.gen(argv_replace_);
    auto cmd_ret = io_ptr_->interactive_shell(invoke_bin_argv_.gen(argv_replace_), wants_stderr);

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
    auto cmd_ret = io_ptr_->interactive_shell(invoke_app_argv_.gen(argv_replace_), false);

    return cmd_ret;
}

MAA_CTRL_UNIT_NS_END
