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

    virtual std::shared_ptr<IOHandler> interactive_shell(const std::string& cmd) override;

    WSADATA wsa_data_ {};
    SOCKET m_server_sock = INVALID_SOCKET;
    sockaddr_in m_server_sock_addr {};
    LPFN_ACCEPTEX m_server_accept_ex = nullptr;

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

class IOHandlerWin32 : public IOHandler
{
public:
    IOHandlerWin32(HANDLE read, HANDLE write, PROCESS_INFORMATION process_info)
        : m_read(read), m_write(write), m_process_info(process_info)
    {}
    IOHandlerWin32(const IOHandlerWin32&) = delete;
    IOHandlerWin32(IOHandlerWin32&&) = delete;
    virtual ~IOHandlerWin32();

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;

private:
    HANDLE m_read = INVALID_HANDLE_VALUE;
    HANDLE m_write = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION m_process_info = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };

    const int PipeBufferSize = 4096;
};

MAA_CTRL_NS_END
#endif
