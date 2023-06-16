#pragma once

#include "Common/MaaConf.h"

#include "Controller/Platform/PlatformIO.h"
#include "Utils/ArgvWrapper.hpp"

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

MAA_CTRL_UNIT_NS_END
