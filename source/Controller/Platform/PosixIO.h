#pragma once

#include "Base/NonCopyable.hpp"
#ifndef _WIN32

#include <netinet/in.h>

#include "Base/SingletonHolder.hpp"
#include "PlatformIO.h"

MAA_CTRL_NS_BEGIN

class PosixIO : public PlatformIO
{
public:
    PosixIO();
    virtual ~PosixIO();

    int call_command(const std::string& cmd, bool recv_by_socket, std::string& pipe_data, std::string& sock_data,
                     int64_t timeout) override;

    std::optional<unsigned short> create_socket(const std::string& local_address) override;
    void close_socket() noexcept override;

    std::shared_ptr<IOHandler> interactive_shell(const std::string& cmd) override;

    int m_server_sock = -1;
    sockaddr_in m_server_sock_addr {};
    static constexpr int PIPE_READ = 0;
    static constexpr int PIPE_WRITE = 1;
    int m_pipe_in[2] = { 0 };
    int m_pipe_out[2] = { 0 };
    int m_child = 0;
};

class IOHandlerPosix : public IOHandler, NonCopyable
{
public:
    IOHandlerPosix(int read_fd, int write_fd, ::pid_t process)
    {
        m_read_fd = read_fd;
        m_write_fd = write_fd;
        m_process = process;
    }

    virtual ~IOHandlerPosix();

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;

private:
    int m_read_fd = -1;
    int m_write_fd = -1;
    ::pid_t m_process = -1;
};

MAA_CTRL_NS_END
#endif
