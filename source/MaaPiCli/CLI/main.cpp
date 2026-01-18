#include <csignal>
#include <format>
#include <iostream>

#include "MaaToolkit/MaaToolkitAPI.h"

#include "MaaUtils/Platform.h"
#include "MaaUtils/Runtime.h"

#include "interactor.h"

void request_windows_emulate_vt100()
{
    // copy from https://github.com/ArthurSonzogni/FTXUI/commit/22afacc28badb35680e7fe03461680c52acbe507
#if defined(_WIN32)
#include "MaaUtils/SafeWindows.hpp"

    // Enable VT processing on stdout and stdin
    auto stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD out_mode = 0;
    GetConsoleMode(stdout_handle, &out_mode);

    // https://docs.microsoft.com/en-us/windows/console/setconsolemode
    const int enable_virtual_terminal_processing = 0x0004;
    const int disable_newline_auto_return = 0x0008;
    out_mode |= enable_virtual_terminal_processing;
    out_mode |= disable_newline_auto_return;

    SetConsoleMode(stdout_handle, out_mode);
#endif
}

extern "C" void sig_handler(int sig)
{
    std::cout << std::format("\nsignal {} received, exit\n", sig) << std::endl;

    exit(0);
}

int main(int argc, char** argv)
{
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGSEGV, sig_handler);
    std::signal(SIGINT, sig_handler);
    std::signal(SIGABRT, sig_handler);

    request_windows_emulate_vt100();

    bool directly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) != "-d") {
            continue;
        }
        directly = true;
        break;
    }

    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());
    std::string resource_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    Interactor interactor(user_path);

    if (!interactor.load(resource_path)) {
        return -1;
    }
    if (directly) {
        interactor.print_config();
        bool result = interactor.run();
        // Note: run() returning true may also mean an elevated instance was started;
        // in either case, we exit the current process gracefully.
        return result ? 0 : -1;
    }

    interactor.interact();
    return 0;
}
