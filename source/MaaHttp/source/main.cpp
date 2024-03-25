#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>

#define BOOST_PROCESS_USE_STD_FS 1
#define BOOST_DLL_USE_STD_FS 1

#include <boost/dll.hpp>
#include <boost/process.hpp>

#include "server.h"

struct SlaveInfo
{
    std::shared_ptr<boost::process::child> child;
    std::chrono::steady_clock::time_point heart;
    std::mutex mtx;

    void update()
    {
        std::lock_guard<std::mutex> lock(mtx);
        heart = std::chrono::steady_clock::now();
    }

    bool isTimeout(unsigned sec = 30)
    {
        std::lock_guard<std::mutex> lock(mtx);
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - heart)
                   .count()
               > sec;
    }
};

int main(int argc, char* argv[])
{
    unsigned port = 0;
    bool slave = false;
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc && !strcmp(argv[i], "-p")) {
            port = atoi(argv[i + 1]);
            if (port > 65535) {
                std::cerr << "invalid port " << argv[i] << std::endl;
                port = 0;
            }
        }
        else if (!strcmp(argv[i], "-s")) {
            slave = true;
        }
    }

    if (slave) {
        lhg::server::Dispatcher* dispatcher;
        lhg::server::Server* server;
        setup_server(static_cast<unsigned short>(port), server, dispatcher);

        port = server->port();

        std::cout << port << std::endl;

        server->sync_run();

        return 0;
    }
    else {
        auto disp = new lhg::server::Dispatcher();
        auto server = new lhg::server::Server(disp, port);

        auto self = boost::dll::program_location();

        bool running = true;
        std::mutex mtx;
        std::map<unsigned short, SlaveInfo> childs;

        std::thread cleaner([&]() {
            while (running) {
                std::unique_lock<std::mutex> lock(mtx);

                for (auto it = childs.begin(); it != childs.end();) {
                    if (it->second.isTimeout()) {
                        std::cerr << "kill slave at " << it->first << std::endl;
                        it->second.child->terminate();
                        it = childs.erase(it);
                    }
                    else {
                        it++;
                    }
                }
                lock.unlock();

                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        });

        disp->handle(
            "/start",
            [&](auto&, auto& res, const auto& req) {
                boost::process::ipstream out_stream;
                auto child = std::make_shared<boost::process::child>(
                    self,
                    std::vector<std::string> { "-s" },
                    boost::process::std_out > out_stream);
                unsigned short slave_port;
                out_stream >> slave_port;
                if (slave_port > 0) {
                    std::unique_lock<std::mutex> lock(mtx);

                    std::cerr << "start slave at " << slave_port << std::endl;
                    auto& slave = childs[slave_port];
                    slave.child = child;
                    slave.update();
                    res = { { "port", slave_port } };
                }
                else {
                    child->terminate();
                    res = { { "port", json::value() } };
                }
            },
            [](auto&, auto&) {});

        disp->handle(
            "/heart",
            [&](auto&, auto& res, const auto& req) {
                std::unique_lock<std::mutex> lock(mtx);

                unsigned short slave_port =
                    static_cast<unsigned short>(req.at("port").as_unsigned());
                if (!childs.contains(slave_port)) {
                    res = { { "success", false }, { "error", std::format("{} not exists", port) } };
                    return;
                }
                childs[slave_port].update();
                res = { { "success", true } };
            },
            [](auto&, auto&) {});

        disp->handle(
            "/stop",
            [&](auto&, auto& res, const auto& req) {
                std::unique_lock<std::mutex> lock(mtx);

                unsigned short port = static_cast<unsigned short>(req.at("port").as_unsigned());
                if (!childs.contains(port)) {
                    res = { { "success", false }, { "error", std::format("{} not exists", port) } };
                    return;
                }
                std::cerr << "kill slave at " << port << std::endl;
                childs[port].child->terminate();
                childs.erase(port);
                res = { { "success", true } };
            },
            [](auto&, auto&) {});

        port = server->port();

        server->run();

        std::cout << "deamon started at 127.0.0.1:" << port << std::endl;

        std::cin.get();

        running = true;

        server->stop();

        cleaner.join();

        return 0;
    }
}
