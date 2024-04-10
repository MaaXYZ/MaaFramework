#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>

#define BOOST_PROCESS_USE_STD_FS 1
#define BOOST_DLL_USE_STD_FS 1

#include <boost/dll.hpp>
#include <boost/process.hpp>

#include "server.h"

struct LaunchInfo
{
    unsigned short port = 0;
    bool slave = false;
};

void parseArgs(int argc, char* argv[], LaunchInfo& info)
{
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc && !strcmp(argv[i], "-p")) {
            auto port = atoi(argv[i + 1]);
            if (port > 65535) {
                std::cerr << "invalid port " << argv[i] << std::endl;
                info.port = 0;
            }
            else {
                info.port = static_cast<unsigned short>(port);
            }
        }
        else if (!strcmp(argv[i], "-s")) {
            info.slave = true;
        }
    }
    if (!info.slave && info.port == 0) {
        info.port = 13126;
    }
}

void slaveMode(unsigned short port)
{
    lhg::server::Dispatcher* dispatcher;
    lhg::server::Server* server;
    setup_server(port, server, dispatcher);

    port = server->port();

    std::cout << port << std::endl;

    server->sync_run();
}

struct SlaveInfo
{
    std::shared_ptr<boost::process::child> child;
    std::shared_ptr<std::thread> stdout_processer;
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

struct MasterMode
{
    bool running = true;
    std::mutex mtx;
    std::map<unsigned short, SlaveInfo> childs;
    std::thread clean_thread;

    MasterMode()
        : clean_thread([this]() { clean(); })
    {
    }

    ~MasterMode()
    {
        running = false;
        clean_thread.join();
    }

    void clean()
    {
        while (running) {
            std::unique_lock<std::mutex> lock(mtx);

            for (auto it = childs.begin(); it != childs.end();) {
                if (it->second.isTimeout()) {
                    std::cerr << "kill slave at " << it->first << std::endl;
                    it->second.child->terminate();
                    it->second.stdout_processer->join();
                    it = childs.erase(it);
                }
                else {
                    it++;
                }
            }
            lock.unlock();

            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }

    std::optional<unsigned short> start()
    {
        auto self = boost::dll::program_location();

        boost::process::ipstream out_stream;
        auto child = std::make_shared<boost::process::child>(
            self,
            std::vector<std::string> { "-s" },
            boost::process::std_out > out_stream);

        std::string slave_port_row;
        std::getline(out_stream, slave_port_row);
        unsigned short slave_port = static_cast<unsigned short>(std::stoul(slave_port_row));

        if (slave_port > 0) {
            auto stdout_processer =
                std::make_shared<std::thread>([os = std::move(out_stream), slave_port]() mutable {
                    std::string row;
                    while (std::getline(os, row)) {
                        std::cout << std::format("[{}] {}", slave_port, row) << std::endl;
                    }
                });

            std::unique_lock<std::mutex> lock(mtx);

            auto& slave = childs[slave_port];
            slave.child = child;
            slave.stdout_processer = stdout_processer;
            slave.update();

            return slave_port;
        }
        else {
            child->terminate();
            return std::nullopt;
        }
    }

    bool heart(unsigned short port)
    {
        std::unique_lock<std::mutex> lock(mtx);

        if (!childs.contains(port)) {
            return false;
        }
        childs[port].update();
        return true;
    }

    bool stop(unsigned short port)
    {
        std::unique_lock<std::mutex> lock(mtx);

        if (!childs.contains(port)) {
            return false;
        }

        std::cerr << "kill slave at " << port << std::endl;

        childs[port].child->terminate();
        childs[port].stdout_processer->join();
        childs.erase(port);

        return true;
    }
};

int main(int argc, char* argv[])
{
    LaunchInfo info;
    parseArgs(argc, argv, info);

    if (info.slave) {
        slaveMode(info.port);
        return 0;
    }
    else {
        auto disp = new lhg::server::Dispatcher();
        auto server = new lhg::server::Server(disp, static_cast<unsigned short>(info.port));

        MasterMode master;

        disp->handle(
            "/start",
            [&](auto&, auto& res, const auto& req) {
                auto port = master.start();
                if (port.has_value()) {
                    std::cerr << "start slave at " << port.value() << std::endl;
                    res = { { "port", port.value() } };
                }
                else {
                    res = { { "port", json::value() } };
                }
            },
            [](auto& req, auto& res) {
                req = {};
                res = lhg::schema::Builder()
                          .properties(json::object {
                              { "port", lhg::schema::Builder().type("number").obj } })
                          .obj;
            });

        disp->handle(
            "/heart",
            [&](auto&, auto& res, const auto& req) {
                if (!req.contains("port")) {
                    res = { { "success", false }, { "error", "port not provided" } };
                    return;
                }
                unsigned short slave_port =
                    static_cast<unsigned short>(req.at("port").as_unsigned());
                if (master.heart(slave_port)) {
                    res = { { "success", true } };
                }
                else {
                    res = { { "success", false },
                            { "error", std::format("{} not exists", slave_port) } };
                }
            },
            [](auto& req, auto& res) {
                req = lhg::schema::Builder()
                          .prop({ { "port", lhg::schema::Builder().type("number").obj } })
                          .obj;
                res = lhg::schema::Builder()
                          .prop({ { "success", lhg::schema::Builder().type("boolean").obj } })
                          .properties({ { "error", lhg::schema::Builder().type("string").obj } })
                          .obj;
            });

        disp->handle(
            "/stop",
            [&](auto&, auto& res, const auto& req) {
                if (!req.contains("port")) {
                    res = { { "success", false }, { "error", "port not provided" } };
                    return;
                }
                unsigned short port = static_cast<unsigned short>(req.at("port").as_unsigned());
                if (master.stop(port)) {
                    std::cerr << "kill slave at " << port << std::endl;
                    res = { { "success", true } };
                }
                else {
                    res = { { "success", false }, { "error", std::format("{} not exists", port) } };
                }
            },
            [](auto& req, auto& res) {
                req = lhg::schema::Builder()
                          .prop({ { "port", lhg::schema::Builder().type("number").obj } })
                          .obj;
                res = lhg::schema::Builder()
                          .prop({ { "success", lhg::schema::Builder().type("boolean").obj } })
                          .properties({ { "error", lhg::schema::Builder().type("string").obj } })
                          .obj;
            });

        auto config_opt = json::open("MaaHttp.json");
        if (config_opt.has_value()) {
            const auto& config = config_opt.value();
            if (config.is_object()) {
                const auto& serve = config.at("serve");
                const auto& redir = config.at("redirect");
                if (serve.is_object()) {
                    for (const auto& [route, base] : serve.as_object()) {
                        if (!base.is_string()) {
                            continue;
                        }
                        std::cout << "serve " << base.as_string() << " at " << route << std::endl;
                        disp->static_serve(
                            route,
                            std::filesystem::current_path() / base.as_string());
                    }
                }
                if (redir.is_object()) {
                    for (const auto& [route, to] : redir.as_object()) {
                        if (!to.is_string()) {
                            continue;
                        }
                        std::cout << "redirect " << route << " to " << to.as_string() << std::endl;
                        disp->redirect(route, to.as_string());
                    }
                }
            }
        }

        disp->setup_help("/help", "maa http monitor", "1.0.0");

        auto port = server->port();

        server->run();

        std::cout << "deamon started at 127.0.0.1:" << port << std::endl;
        std::cin.get();

        server->stop();

        return 0;
    }
}
