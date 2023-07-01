#ifdef _WIN32
#include "Win32IO.h"

#include <ws2tcpip.h>

#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

MAA_CTRL_UNIT_NS_BEGIN

Win32IO::Win32IO()
{
    // 现在暂时没用上这个 bool，但是还是要利用下 WsaHelper 的构造完成 WSAStartup
    support_socket_ = WsaHelper::get_instance()();
}

Win32IO::~Win32IO()
{
    if (server_sock_ != INVALID_SOCKET) {
        ::closesocket(server_sock_);
        server_sock_ = INVALID_SOCKET;
    }
}

int Win32IO::call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                          std::string& sock_data, int64_t timeout)
{
    using namespace std::chrono;

    auto start_time = std::chrono::steady_clock::now();

    MAA_PLATFORM_NS::single_page_buffer<char> pipe_buffer;
    MAA_PLATFORM_NS::single_page_buffer<char> sock_buffer;

    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa_inherit { .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE };
    if (!CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_inherit, (DWORD)pipe_buffer.size(),
                                true, false)) {
        DWORD err = GetLastError();
        LogError << "CreateOverlappablePipe failed" << VAR(err);
        return -1;
    }

    STARTUPINFOW si {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;
    PROCESS_INFORMATION process_info = { nullptr }; // 进程信息结构体

    std::vector<os_string> ocmd;
    std::transform(cmd.begin(), cmd.end(), std::back_insert_iterator(ocmd), to_osstring);
    auto cmdline_osstr = args_to_cmd(ocmd);
    BOOL create_ret =
        CreateProcessW(nullptr, cmdline_osstr.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &process_info);
    if (!create_ret) {
        DWORD err = GetLastError();
        LogError << "CreateProcessW failed" << VAR(cmd) << VAR(create_ret) << VAR(err);
        return -1;
    }

    CloseHandle(pipe_child_write);
    pipe_child_write = INVALID_HANDLE_VALUE;

    std::vector<HANDLE> wait_handles;
    wait_handles.reserve(3);
    bool process_running = true;
    bool pipe_eof = false;
    bool accept_pending = false;
    bool socket_eof = false;

    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    (void)ReadFile(pipe_parent_read, pipe_buffer.get(), (DWORD)pipe_buffer.size(), nullptr, &pipeov);

    OVERLAPPED sockov {};
    SOCKET client_socket = INVALID_SOCKET;

    if (recv_by_socket) {
        sock_buffer = MAA_PLATFORM_NS::single_page_buffer<char>();
        sockov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        DWORD dummy;
        if (!server_accept_ex_(server_sock_, client_socket, sock_buffer.get(),
                               (DWORD)sock_buffer.size() - ((sizeof(sockaddr_in) + 16) * 2), sizeof(sockaddr_in) + 16,
                               sizeof(sockaddr_in) + 16, &dummy, &sockov)) {
            DWORD err = WSAGetLastError();
            if (err == ERROR_IO_PENDING) {
                accept_pending = true;
            }
            else {
                LogError << "AcceptEx failed" << VAR(err);
                accept_pending = false;
                socket_eof = true;
                ::closesocket(client_socket);
            }
        }
    }

    while (true) { // TODO: !need_exit()
        wait_handles.clear();
        if (process_running) wait_handles.push_back(process_info.hProcess);
        if (!pipe_eof) wait_handles.push_back(pipeov.hEvent);
        if (recv_by_socket && ((accept_pending && process_running) || !socket_eof)) {
            wait_handles.push_back(sockov.hEvent);
        }
        if (wait_handles.empty()) break;
        auto elapsed = steady_clock::now() - start_time;
        // TODO: 这里目前是隔 5000ms 判断一次，应该可以加一个 wait_handle 来判断外部中断（need_exit）
        auto wait_time =
            (std::min)(timeout - duration_cast<milliseconds>(elapsed).count(), process_running ? 5LL * 1000 : 0LL);
        if (wait_time < 0) break;
        auto wait_result =
            WaitForMultipleObjectsEx((DWORD)wait_handles.size(), wait_handles.data(), FALSE, (DWORD)wait_time, TRUE);
        HANDLE signaled_object = INVALID_HANDLE_VALUE;
        if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + wait_handles.size()) {
            signaled_object = wait_handles[(size_t)wait_result - WAIT_OBJECT_0];
        }
        else if (wait_result == WAIT_TIMEOUT) {
            if (wait_time == 0) {
                std::vector<std::string> handle_string {};
                for (auto handle : wait_handles) {
                    if (handle == process_info.hProcess) {
                        handle_string.emplace_back("process_info.hProcess");
                    }
                    else if (handle == pipeov.hEvent) {
                        handle_string.emplace_back("pipeov.hEvent");
                    }
                    else if (recv_by_socket && handle == sockov.hEvent) {
                        handle_string.emplace_back("sockov.hEvent");
                    }
                    else {
                        handle_string.emplace_back("UnknownHandle");
                    }
                }
                LogWarn << "Wait handles timeout" << VAR(handle_string);
                if (process_running) {
                    TerminateProcess(process_info.hProcess, 0);
                }
                break;
            }
            continue;
        }
        else {
            // something bad happened
            DWORD err = GetLastError();
            // throw std::system_error(std::error_code(err, std::system_category()));
            LogError << "A fatal error occurred" << VAR(err);
            break;
        }

        if (signaled_object == process_info.hProcess) {
            process_running = false;
        }
        else if (signaled_object == pipeov.hEvent) {
            // pipe read
            DWORD len = 0;
            if (GetOverlappedResult(pipe_parent_read, &pipeov, &len, FALSE)) {
                pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + len);
                (void)ReadFile(pipe_parent_read, pipe_buffer.get(), (DWORD)pipe_buffer.size(), nullptr, &pipeov);
            }
            else {
                DWORD err = GetLastError();
                if (err == ERROR_HANDLE_EOF || err == ERROR_BROKEN_PIPE) {
                    pipe_eof = true;
                }
            }
        }
        else if (signaled_object == sockov.hEvent) {
            if (accept_pending) {
                // AcceptEx, client_socker is connected and first chunk of data is received
                DWORD len = 0;
                if (GetOverlappedResult(reinterpret_cast<HANDLE>(server_sock_), &sockov, &len, FALSE)) {
                    accept_pending = false;
                    if (recv_by_socket) sock_data.insert(sock_data.end(), sock_buffer.get(), sock_buffer.get() + len);

                    if (len == 0) {
                        socket_eof = true;
                        ::closesocket(client_socket);
                    }
                    else {
                        // reset the overlapped since we reuse it for different handle
                        auto event = sockov.hEvent;
                        sockov = {};
                        sockov.hEvent = event;

                        (void)ReadFile(reinterpret_cast<HANDLE>(client_socket), sock_buffer.get(),
                                       (DWORD)sock_buffer.size(), nullptr, &sockov);
                    }
                }
            }
            else {
                // ReadFile
                DWORD len = 0;
                if (GetOverlappedResult(reinterpret_cast<HANDLE>(client_socket), &sockov, &len, FALSE)) {
                    if (recv_by_socket) sock_data.insert(sock_data.end(), sock_buffer.get(), sock_buffer.get() + len);
                    if (len == 0) {
                        socket_eof = true;
                        ::closesocket(client_socket);
                    }
                    else {
                        (void)ReadFile(reinterpret_cast<HANDLE>(client_socket), sock_buffer.get(),
                                       (DWORD)sock_buffer.size(), nullptr, &sockov);
                    }
                }
                else {
                    // err = GetLastError();
                    socket_eof = true;
                    ::closesocket(client_socket);
                }
            }
        }
    }

    DWORD exit_ret = 0;
    GetExitCodeProcess(process_info.hProcess, &exit_ret);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    CloseHandle(pipe_parent_read);
    CloseHandle(pipeov.hEvent);
    if (recv_by_socket) {
        if (!socket_eof) closesocket(client_socket);
        CloseHandle(sockov.hEvent);
    }

    return static_cast<int>(exit_ret);
}

std::optional<unsigned short> Win32IO::create_socket(const std::string& local_address)
{
    LogFunc << VAR(local_address);

    if (server_sock_ == INVALID_SOCKET) {
        server_sock_ = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_sock_ == INVALID_SOCKET) {
            return std::nullopt;
        }
    }

    DWORD dummy = 0;
    GUID guid_accept_ex = WSAID_ACCEPTEX;
    int err = WSAIoctl(server_sock_, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_accept_ex, sizeof(guid_accept_ex),
                       &server_accept_ex_, sizeof(server_accept_ex_), &dummy, NULL, NULL);
    if (err == SOCKET_ERROR) {
        err = WSAGetLastError();
        LogError << "failed to resolve AcceptEx" << VAR(err);
        ::closesocket(server_sock_);
        return std::nullopt;
    }
    server_sock_addr_.sin_family = PF_INET;
    ::inet_pton(AF_INET, local_address.c_str(), &server_sock_addr_.sin_addr);

    bool server_start = false;
    uint16_t port_result = 0;

    server_sock_addr_.sin_port = ::htons(0);
    int bind_ret = ::bind(server_sock_, reinterpret_cast<SOCKADDR*>(&server_sock_addr_), sizeof(SOCKADDR));
    int addrlen = sizeof(server_sock_addr_);
    int getname_ret = ::getsockname(server_sock_, reinterpret_cast<sockaddr*>(&server_sock_addr_), &addrlen);
    int listen_ret = ::listen(server_sock_, 3);
    server_start = bind_ret == 0 && getname_ret == 0 && listen_ret == 0;

    if (!server_start) {
        LogInfo << "not supports socket";
        return std::nullopt;
    }

    port_result = ::ntohs(server_sock_addr_.sin_port);

    LogInfo << "command server start" << VAR(local_address) << VAR(port_result);
    return port_result;
}

void Win32IO::close_socket() noexcept
{
    if (server_sock_ != INVALID_SOCKET) {
        ::closesocket(server_sock_);
        server_sock_ = INVALID_SOCKET;
    }
}

std::shared_ptr<IOHandler> Win32IO::tcp(const std::string& target, unsigned short port)
{
    SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in sock_addr = {};
    sock_addr.sin_family = PF_INET;
    inet_pton(AF_INET, target.c_str(), &sock_addr.sin_addr);
    sock_addr.sin_port = htons(port);
    auto ret = connect(sock, (SOCKADDR*)&sock_addr, sizeof(SOCKADDR));
    if (ret < 0) {
        LogError << "failed to connect" << VAR(target) << VAR(port);
        return nullptr;
    }
    return std::make_shared<SocketIOHandlerWin32>(sock);
}

std::shared_ptr<IOHandler> Win32IO::interactive_shell(const std::vector<std::string>& cmd)
{
    constexpr int PipeReadBuffSize = 4096ULL;
    constexpr int PipeWriteBuffSize = 64 * 1024ULL;

    SECURITY_ATTRIBUTES sa_attr_inherit {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };
    PROCESS_INFORMATION process_info_ = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    HANDLE pipe_child_read = INVALID_HANDLE_VALUE, pipe_parent_write = INVALID_HANDLE_VALUE;
    if (!CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_attr_inherit, PipeReadBuffSize, true,
                                false) ||
        !CreateOverlappablePipe(&pipe_child_read, &pipe_parent_write, &sa_attr_inherit, nullptr, PipeWriteBuffSize,
                                false, false)) {
        DWORD err = GetLastError();
        LogError << "Failed to create pipe for minitouch" << VAR(err);
        return nullptr;
    }

    STARTUPINFOW si {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = pipe_child_read;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;

    std::vector<os_string> ocmd;
    std::transform(cmd.begin(), cmd.end(), std::back_insert_iterator(ocmd), to_osstring);
    auto cmd_osstr = args_to_cmd(ocmd);
    BOOL create_ret =
        CreateProcessW(NULL, cmd_osstr.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &process_info_);
    CloseHandle(pipe_child_write);
    CloseHandle(pipe_child_read);
    pipe_child_write = INVALID_HANDLE_VALUE;
    pipe_child_read = INVALID_HANDLE_VALUE;

    if (!create_ret) {
        DWORD err = GetLastError();
        LogError << "Failed to create process for minitouch" << VAR(create_ret) << VAR(err);
        CloseHandle(process_info_.hProcess);
        CloseHandle(process_info_.hThread);
        CloseHandle(pipe_parent_read);
        CloseHandle(pipe_parent_write);
        return nullptr;
    }

    return std::make_shared<PipeIOHandlerWin32>(pipe_parent_read, pipe_parent_write, process_info_);
}

bool Win32IO::CreateOverlappablePipe(HANDLE* read, HANDLE* write, SECURITY_ATTRIBUTES* secattr_read,
                                     SECURITY_ATTRIBUTES* secattr_write, DWORD bufsize, bool overlapped_read,
                                     bool overlapped_write)
{
    static std::atomic<size_t> pipeid {};
    auto pipename = std::format(L"\\\\.\\pipe\\maa-pipe-{}-{}", GetCurrentProcessId(), pipeid++);
    DWORD read_flag = PIPE_ACCESS_INBOUND;
    if (overlapped_read) read_flag |= FILE_FLAG_OVERLAPPED;
    DWORD write_flag = GENERIC_WRITE;
    if (overlapped_write) write_flag |= FILE_FLAG_OVERLAPPED;
    auto pipe_read =
        CreateNamedPipeW(pipename.c_str(), read_flag, PIPE_TYPE_BYTE | PIPE_WAIT, 1, bufsize, bufsize, 0, secattr_read);
    if (pipe_read == INVALID_HANDLE_VALUE) return false;
    auto pipe_write =
        CreateFileW(pipename.c_str(), write_flag, 0, secattr_write, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pipe_write == INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_read);
        return false;
    }

    *read = pipe_read;
    *write = pipe_write;
    return true;
}

PipeIOHandlerWin32::~PipeIOHandlerWin32()
{
    if (process_info_.hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(process_info_.hProcess);
        process_info_.hProcess = INVALID_HANDLE_VALUE;
    }
    if (process_info_.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(process_info_.hThread);
        process_info_.hThread = INVALID_HANDLE_VALUE;
    }
    if (read_ != INVALID_HANDLE_VALUE) {
        CloseHandle(read_);
        read_ = INVALID_HANDLE_VALUE;
    }
    if (write_ != INVALID_HANDLE_VALUE) {
        CloseHandle(write_);
        write_ = INVALID_HANDLE_VALUE;
    }
}

std::string PipeIOHandlerWin32::read(unsigned timeout_sec)
{
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    auto pipe_buffer = std::make_unique<char[]>(PipeBufferSize);
    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    std::ignore = ReadFile(read_, pipe_buffer.get(), PipeBufferSize, nullptr, &pipeov);

    while (true) {
        if (!check_timeout(start_time)) {
            CancelIoEx(read_, &pipeov);
            LogError << "read timeout";
            break;
        }
        DWORD len = 0;
        if (GetOverlappedResult(read_, &pipeov, &len, FALSE)) {
            break;
        }
    }

    return pipe_buffer.get();
}

std::string PipeIOHandlerWin32::read(unsigned timeout_sec, size_t expect)
{
    // TODO
    std::ignore = timeout_sec, expect;
    return std::string();
}

bool PipeIOHandlerWin32::write(std::string_view data)
{
    if (write_ == INVALID_HANDLE_VALUE) {
        LogError << "IOHandler write handle invalid";
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(write_, data.data(), static_cast<DWORD>(data.size() * sizeof(std::string::value_type)), &written,
                   NULL)) {
        auto err = GetLastError();
        LogError << "Failed to write to minitouch" << VAR(err);
        return false;
    }

    return data.size() == written;
}

SocketIOHandlerWin32::~SocketIOHandlerWin32()
{
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

bool SocketIOHandlerWin32::write(std::string_view data)
{
    if (socket_ == INVALID_SOCKET) {
        return false;
    }
    if (::send(socket_, data.data(), static_cast<int>(data.length()), 0) >= 0) {
        return true;
    }
    LogError << "Failed to send to SocketIOHandlerWin32, err" << WSAGetLastError();
    return false;
}

std::string SocketIOHandlerWin32::read(unsigned timeout_sec)
{
    if (socket_ == INVALID_SOCKET) {
        return "";
    }
    std::string ret_str;
    constexpr int PipeReadBuffSize = 4096ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        char buf_from_child[PipeReadBuffSize];

        if (!check_timeout(start_time)) {
            break;
        }

        auto ret_read = ::recv(socket_, buf_from_child, PipeReadBuffSize, 0);
        if (ret_read > 0) {
            ret_str.insert(ret_str.end(), buf_from_child, buf_from_child + ret_read);
        }
        else {
            break;
        }
    }
    return ret_str;
}

std::string SocketIOHandlerWin32::read(unsigned timeout_sec, size_t expect)
{
    if (socket_ == INVALID_SOCKET) {
        return "";
    }
    std::string ret_str;
    constexpr size_t PipeReadBuffSize = 4096ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        char buf_from_child[PipeReadBuffSize];

        if (!check_timeout(start_time)) {
            break;
        }

        auto ret_read =
            ::recv(socket_, buf_from_child, static_cast<int>(std::min(PipeReadBuffSize, expect - ret_str.size())), 0);
        if (ret_read > 0) {
            ret_str.insert(ret_str.end(), buf_from_child, buf_from_child + ret_read);
        }
        else {
            // break;
        }
    }
    return ret_str;
}

MAA_CTRL_UNIT_NS_END

#endif //_WIN32
