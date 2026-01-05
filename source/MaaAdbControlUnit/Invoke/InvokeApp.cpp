#include "InvokeApp.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/Time.hpp"

MAA_CTRL_UNIT_NS_BEGIN

InvokeApp::~InvokeApp()
{
    remove();
}

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
        "export LD_LIBRARY_PATH=/data/local/tmp/; \"/data/local/tmp/{BIN_WORKING_FILE}\" "
        "{BIN_EXTRA_PARAMS} 2>&1",
    };
    static const json::array kDefaultInvokeAppArgv = {
        "{ADB}",
        "-s",
        "{ADB_SERIAL}",
        "shell",
        "export CLASSPATH=\"/data/local/tmp/{APP_WORKING_FILE}\"; app_process /data/local/tmp "
        "{PACKAGE_NAME}",
    };
    static const json::array kDefaultRemoveFileArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "rm \"/data/local/tmp/{TO_REMOVED_FILE}\"",
    };
    return parse_command("Abilist", config, kDefaultAbilistArgv, abilist_argv_) && parse_command("SDK", config, kDefaultSdkArgv, sdk_argv_)
           && parse_command("PushBin", config, kDefaultPushBinArgv, push_bin_argv_)
           && parse_command("ChmodBin", config, kDefaultChmodBinArgv, chmod_bin_argv_)
           && parse_command("InvokeBin", config, kDefaultInvokeBinArgv, invoke_bin_argv_)
           && parse_command("InvokeApp", config, kDefaultInvokeAppArgv, invoke_app_argv_)
           && parse_command("RemoveFile", config, kDefaultRemoveFileArgv, remove_file_argv_);
}

bool InvokeApp::init(const std::string& force_temp)
{
    tempname_ = force_temp.empty() ? format_now_for_filename() : force_temp;

    LogDebug << VAR(tempname_);

    return true;
}

std::optional<std::vector<std::string>> InvokeApp::abilist()
{
    LogFunc;

    auto argv_opt = abilist_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    return string_split(*output_opt, ',');
}

std::optional<int> InvokeApp::sdk()
{
    LogFunc;

    auto argv_opt = sdk_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    std::string& ret = *output_opt;
    string_trim_(ret);

    if (!std::ranges::all_of(ret, [](unsigned char c) { return std::isdigit(c); })) {
        return std::nullopt;
    }
    return std::stoi(ret);
}

bool InvokeApp::push(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    std::string absolute_path = path_to_utf8_string(std::filesystem::absolute(path));
    merge_replacement({ { "{BIN_PATH}", absolute_path }, { "{BIN_WORKING_FILE}", tempname_ } });

    auto argv_opt = push_bin_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return false;
    }

    pushed_ = true;
    return true;
}

bool InvokeApp::chmod()
{
    LogFunc;

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ } });

    auto argv_opt = chmod_bin_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return false;
    }

    return true;
}

bool InvokeApp::remove()
{
    LogFunc;

    if (!pushed_) {
        LogDebug << "Not pushed, skip remove";
        return true;
    }
    if (tempname_.empty()) {
        LogWarn << "tempname_ is empty";
        return false;
    }

    merge_replacement({ { "{TO_REMOVED_FILE}", tempname_ } });

    auto argv_opt = remove_file_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return false;
    }

    pushed_ = false;
    return true;
}

std::optional<std::string> InvokeApp::invoke_bin_and_read_pipe(const std::string& extra)
{
    LogFunc << VAR(extra);

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ }, { "{BIN_EXTRA_PARAMS}", extra } });

    auto argv_opt = invoke_bin_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    return startup_and_read_pipe(*argv_opt);
}

std::shared_ptr<ChildPipeIOStream> InvokeApp::invoke_bin(const std::string& extra)
{
    LogFunc << VAR(extra);

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ }, { "{BIN_EXTRA_PARAMS}", extra } });

    auto argv_opt = invoke_bin_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return nullptr;
    }

    return std::make_shared<ChildPipeIOStream>(argv_opt->exec, argv_opt->args);
}

std::shared_ptr<ChildPipeIOStream> InvokeApp::invoke_app(const std::string& package)
{
    LogFunc << VAR(package);

    merge_replacement({ { "{APP_WORKING_FILE}", tempname_ }, { "{PACKAGE_NAME}", package } });

    auto argv_opt = invoke_app_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return nullptr;
    }

    return std::make_shared<ChildPipeIOStream>(argv_opt->exec, argv_opt->args);
}

MAA_CTRL_UNIT_NS_END
