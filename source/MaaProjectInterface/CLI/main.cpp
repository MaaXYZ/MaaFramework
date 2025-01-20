#include <csignal>
#include <format>
#include <iostream>

#include "Utils/Platform.h"
#include "Utils/Runtime.h"

#include "MaaToolkit/ProjectInterface/MaaToolkitProjectInterface.h"

extern "C" void sig_handler(int sig)
{
    std::cout << std::format("\nsignal {} received, exit\n", sig);

    exit(0);
}

int main(int argc, char** argv)
{
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGSEGV, sig_handler);
    std::signal(SIGINT, sig_handler);
    std::signal(SIGABRT, sig_handler);

    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());
    std::string resource_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    bool directly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) != "-d") {
            continue;
        }
        directly = true;
        break;
    }

    bool ret = MaaToolkitProjectInterfaceRunCli(0, resource_path.c_str(), user_path.c_str(), directly, nullptr, nullptr);
    return ret ? 0 : -1;
}
