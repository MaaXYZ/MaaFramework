#pragma once

#include <filesystem>

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class Connection : public UnitBase
{
public:
    Connection(std::filesystem::path adb_path, std::string adb_serial);
    virtual ~Connection() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    bool connect();
    bool kill_server();
    bool test_connection();

private:
    bool connect_remote();

    std::filesystem::path adb_path_;
    std::string adb_serial_;

    ProcessArgvGenerator connect_argv_;
    ProcessArgvGenerator kill_server_argv_;
    ProcessArgvGenerator test_connection_argv_;
};

MAA_CTRL_UNIT_NS_END
