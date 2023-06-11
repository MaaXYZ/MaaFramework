#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include "Controller/Platform/PlatformIO.h"

#include <meojson/json.hpp>
#include <unordered_map>

#define MAA_CTRL_UNIT_NS MAA_CTRL_NS::Unit
#define MAA_CTRL_UNIT_NS_BEGIN \
    namespace MAA_CTRL_UNIT_NS \
    {
#define MAA_CTRL_UNIT_NS_END }

MAA_CTRL_UNIT_NS_BEGIN

class UnitHelper : NonCopyable
{
public:
    using Argv = std::vector<std::string>;

public:
    UnitHelper(std::shared_ptr<PlatformIO> io_ptr);

    void set_replacement(std::map<std::string, std::string> argv_replace);

protected:
    static bool parse_argv(const std::string& key, const json::value& config, /*out*/ Argv& argv);

    std::optional<std::string> command(const Argv& cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::shared_ptr<PlatformIO> io_ptr_;
    std::map<std::string, std::string> argv_replace_;
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

MAA_CTRL_UNIT_NS_END
