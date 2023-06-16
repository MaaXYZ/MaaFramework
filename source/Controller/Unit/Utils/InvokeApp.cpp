#include "InvokeApp.h"

#include "../utils.h"
#include "Utils/Logger.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool InvokeApp::parse(const json::value& config)
{
    return parse_argv("Abilist", config, abilist_argv_) && parse_argv("SDK", config, sdk_argv_) &&
           parse_argv("PushBin", config, push_bin_argv_) && parse_argv("ChmodBin", config, chmod_bin_argv_) &&
           parse_argv("InvokeBin", config, invoke_bin_argv_) && parse_argv("InvokeApp", config, invoke_app_argv_);
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

std::optional<int> InvokeApp::sdk()
{
    LogFunc;

    auto cmd_ret = command(sdk_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return atoi(cmd_ret.value().c_str());
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

std::optional<std::string> InvokeApp::invoke_bin_stdout(const std::string& extra)
{
    LogFunc;

    merge_replacement({ { "{BIN_WORKING_FILE}", tempname_ }, { "{BIN_EXTRA_PARAMS}", extra } });
    LogInfo << invoke_bin_argv_.gen(argv_replace_);
    return command(invoke_bin_argv_.gen(argv_replace_));
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
