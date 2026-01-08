#pragma once

#include <filesystem>
#include <memory>

#include "Base/UnitBase.h"
#include "MaaUtils/IOStream/ChildPipeIOStream.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class Connection : public UnitBase
{
public:
    Connection(std::filesystem::path adb_path, std::string adb_serial);
    virtual ~Connection() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    bool connect();
    bool kill_server();
    bool test_connection();

    // 通过监控进程检测连接状态，零开销
    bool is_alive() const;

private:
    bool connect_remote();
    bool start_monitor();
    void stop_monitor();

    std::filesystem::path adb_path_;
    std::string adb_serial_;

    ProcessArgvGenerator connect_argv_;
    ProcessArgvGenerator kill_server_argv_;
    ProcessArgvGenerator test_connection_argv_;
    ProcessArgvGenerator monitor_argv_;

    // 持久化的监控进程，设备断开时自动退出
    std::shared_ptr<ChildPipeIOStream> monitor_ios_;
};

MAA_CTRL_UNIT_NS_END
