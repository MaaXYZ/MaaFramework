#pragma once
#ifndef _WIN32

#include "Base/NonCopyable.hpp"

#include <netinet/in.h>

#include "Base/SingletonHolder.hpp"
#include "PlatformIO.h"

MAA_CTRL_NS_BEGIN

class MAA_DEBUG_API PosixIO : public PlatformIO
{
public:
    PosixIO();
    virtual ~PosixIO();

    int call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                     std::string& sock_data, int64_t timeout) override;

    std::optional<unsigned short> create_socket(const std::string& local_address) override;
    void close_socket() noexcept override;

    std::shared_ptr<IOHandler> tcp(const std::string& target, unsigned short port) override;
    std::shared_ptr<IOHandler> interactive_shell(const std::vector<std::string>& cmd) override;

    int server_sock_ = -1;
    sockaddr_in server_sock_addr_ {};
    static constexpr int PIPE_READ = 0;
    static constexpr int PIPE_WRITE = 1;
    int pipe_in_[2] = { 0 };
    int pipe_out_[2] = { 0 };
    int child_ = 0;
};

class IOHandlerPosix : public IOHandler, NonCopyable
{
public:
    IOHandlerPosix(int read_fd, int write_fd, ::pid_t process)
        : read_fd_(read_fd), write_fd_(write_fd), process_(process)
    {}

    virtual ~IOHandlerPosix();

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;
    virtual std::string read(unsigned timeout_sec, size_t expect) override;

private:
    int read_fd_ = -1;
    int write_fd_ = -1;
    ::pid_t process_ = -1;
};

MAA_CTRL_NS_END
#endif
