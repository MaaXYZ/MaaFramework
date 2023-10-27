
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "MaaRpc/MaaRpc.h"

std::mutex mutex;
std::condition_variable cv;
bool mi = false;
bool quiet = false;
bool quit = false;

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
        else if (opt == "-mi") {
            mi = true;
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
    if (!MaaRpcStart(server_address.c_str())) {
        if (!quiet) {
            if (mi) {
                std::cout << "[MAARPC]ERROR" << std::endl;
            }
            else {
                std::cout << "Server failed to listen on " << server_address << std::endl;
            }
        }
        return 1;
    }

    std::unique_lock<std::mutex> lock(mutex);
    if (!quiet) {
        if (mi) {
            std::cout << "[MAARPC]START|" << server_address << std::endl;
        }
        else {
            std::cout << "Server listening on " << server_address << std::endl;
        }
    }

    std::thread wait_enter([]() {
        std::string row;
        std::getline(std::cin, row);
        quit = true;
        std::unique_lock<std::mutex> lock(mutex);
        cv.notify_all();
    });

    cv.wait(lock, []() { return quit; });

    if (mi) {
        std::cout << "[MAARPC]STOP" << std::endl;
    }
    else {
        std::cout << "Start stopping" << std::endl;
    }
    MaaRpcStop();

    if (mi) {
        std::cout << "[MAARPC]EXIT" << std::endl;
    }
    else {
        std::cout << "Exit" << std::endl;
    }
    wait_enter.join();
    return 0;
}
