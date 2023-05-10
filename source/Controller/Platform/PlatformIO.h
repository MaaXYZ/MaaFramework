#pragma once

#include "Common/MaaConf.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>

MAA_CTRL_NS_BEGIN

enum class PlatformType
{
    Native,
    AdbLite
};

class IOHandler;

class IPlatformRelatedInstanceInfo
{
public:
    virtual bool need_exit() const = 0;
};

class PlatformIO
{
public:
    virtual ~PlatformIO() = default;
    
    virtual std::optional<int> call_command(const std::string& cmd, bool recv_by_socket, std::string& pipe_data,
                                            std::string& sock_data, int64_t timeout,
                                            std::chrono::steady_clock::time_point start_time) = 0;

    virtual std::optional<unsigned short> init_socket(const std::string& local_address) = 0;
    virtual void close_socket() noexcept = 0;

    virtual std::shared_ptr<IOHandler> interactive_shell(const std::string& cmd) = 0;

    virtual void release_adb(const std::string& adb_release, int64_t timeout = 20000) = 0;

    bool m_support_socket = false;
};

class IOHandler
{
public:
    virtual ~IOHandler() = default;

    virtual bool write(std::string_view data) = 0;
    virtual std::string read(unsigned timeout_sec) = 0;
};

MAA_CTRL_NS_END
