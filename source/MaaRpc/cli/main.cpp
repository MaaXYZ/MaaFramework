
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>

#include "MaaRpc/MaaRpc.h"

std::mutex mutex;
std::condition_variable cv;
bool quiet = false;

void sig_handler(int)
{
    if (!quiet) {
        std::cout << "Quit from interrupt" << std::endl;
    }
    std::unique_lock<std::mutex> lock(mutex);
    cv.notify_all();
}

int main(int argc, char* argv[])
{
    std::string host = "localhost";
    int port = 8080;

    for (int i = 1; i < argc; i++) {
        std::string opt = argv[i];
        if (opt == "-v" || opt == "--version") {
            std::cout << "MaaRpcCli version " << MAA_VERSION << std::endl;
            return 0;
        }
        else if (opt == "-h" || opt == "--help") {
            std::cout << "Usage: " << argv[0] << " [-q] [-p=PORT] [-h=HOST]" << std::endl;
            return 0;
        }
        else if (opt == "-q") {
            quiet = true;
        }
        else if (opt.starts_with("-p=")) {
            port = std::stoi(opt.substr(3));
        }
        else if (opt.starts_with("-h=")) {
            host = opt.substr(3);
        }
        else {
            std::cerr << "Unknown option " << opt << std::endl;
            return -1;
        }
    }
    std::string server_address(host + ":" + std::to_string(port));
    MaaRpcStart(server_address.c_str());
    signal(SIGINT, sig_handler);

    std::unique_lock<std::mutex> lock(mutex);
    if (!quiet) {
        std::cout << "Server listening on " << server_address << std::endl;
    }
    cv.wait(lock);

    MaaRpcStop();

    return 0;
}
