#pragma once

#include "Common/MaaConf.h"
#include "MaaPort.h"

#include "Controller/Platform/PlatformIO.h"
#include "Utils/ArgvWrapper.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI_DEBUG UnitBase
{
public:
    using Argv = ArgvWrapper<std::vector<std::string>>;

public:
    virtual ~UnitBase() = default;

    virtual bool parse(const json::value& config) = 0;

    void set_io(std::shared_ptr<PlatformIO> io_ptr);
    void set_replacement(Argv::replacement argv_replace);
    void merge_replacement(Argv::replacement argv_replace, bool _override = true);

protected:
    static bool parse_argv(const std::string& key, const json::value& config, /*out*/ Argv& argv);

    std::optional<std::string> command(Argv::value cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::shared_ptr<PlatformIO> io_ptr_ = nullptr;
    std::vector<std::shared_ptr<UnitBase>> children_;
    Argv::replacement argv_replace_;
};

MAA_CTRL_UNIT_NS_END
