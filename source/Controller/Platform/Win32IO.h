#pragma once

#ifdef _WIN32
#include "PlatformIO.h"

#include "Utils/Platform/SafeWindows.h"
#include <mswsock.h>

#include "Base/SingletonHolder.hpp"

MAA_CTRL_NS_BEGIN

class Win32IO : public PlatformIO
{
public:
    Win32IO();
    virtual ~Win32IO();

    virtual int call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                             std::string& sock_data, int64_t timeout) override;

    virtual std::optional<unsigned short> create_socket(const std::string& local_address) override;
    virtual void close_socket() noexcept override;

    virtual std::shared_ptr<IOHandler> tcp(const std::string& target, unsigned short port) override;
    virtual std::shared_ptr<IOHandler> interactive_shell(const std::vector<std::string>& cmd) override;

    WSADATA wsa_data_ {};
    SOCKET server_sock_ = INVALID_SOCKET;
    sockaddr_in server_sock_addr_ {};
    LPFN_ACCEPTEX server_accept_ex_ = nullptr;

private:
    // for Windows socket
    class WsaHelper : public SingletonHolder<WsaHelper>
    {
        friend class SingletonHolder<WsaHelper>;

    public:
        virtual ~WsaHelper() override { WSACleanup(); }
        bool operator()() const noexcept { return supports_; }

    private:
        WsaHelper() { supports_ = WSAStartup(MAKEWORD(2, 2), &wsa_data_) == 0 && wsa_data_.wVersion == MAKEWORD(2, 2); }
        WSADATA wsa_data_ = { 0 };
        bool supports_ = false;
    };
};

class PipeIOHandlerWin32 : public IOHandler, public NonCopyable
{
public:
    PipeIOHandlerWin32(HANDLE read, HANDLE write, PROCESS_INFORMATION process_info)
        : read_(read), write_(write), process_info_(process_info)
    {}
    virtual ~PipeIOHandlerWin32();

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;
    virtual std::string read(unsigned timeout_sec, size_t expect) override;

private:
    HANDLE read_ = INVALID_HANDLE_VALUE;
    HANDLE write_ = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION process_info_ = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };

    const int PipeBufferSize = 4096;
};

class SocketIOHandlerWin32 : public IOHandler, public NonCopyable
{
public:
    SocketIOHandlerWin32(SOCKET socket) : socket_(socket) {}

    virtual ~SocketIOHandlerWin32();
    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;
    virtual std::string read(unsigned timeout_sec, size_t expect) override;

private:
    SOCKET socket_ = INVALID_SOCKET;
};

MAA_CTRL_NS_END
#endif
