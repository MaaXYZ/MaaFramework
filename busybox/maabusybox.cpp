#define _CRT_SECURE_NO_WARNINGS

#include "Controller/Platform/PlatformFactory.h"
#include "Controller/Unit/ControlUnit.h"
#include "MaaAPI.h"
#include "Utils/ArgvWrapper.hpp"
#include "Utils/NoWarningCV.h"
#include "cxxopts.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>

std::ostream& operator<<(std::ostream& os, const MaaNS::ControllerNS::Unit::DeviceInfo::Resolution& res)
{
    return os << "{ width: " << res.width << ", height: " << res.height << " }";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    os << "[";
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(os, ", "));
    return os << "]";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    if (v.has_value()) {
        return os << v.value();
    }
    else {
        return os << "<nullopt>";
    }
}

inline std::string read_adb_argv(const std::string& cur_dir)
{
    std::ifstream ifs(std::filesystem::path(cur_dir) / "adb_argv.json", std::ios::in);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open adb_argv.json\n"
                  << "Please copy adb_argv.json to " << cur_dir << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

// 用string防止后续的replacement触发explicit构造函数
std::map<std::string_view, std::string> intents = {
    { "Official", "com.hypergryph.arknights/com.u8.sdk.U8UnityContext" },
    { "Bilibili", "com.hypergryph.arknights.bilibili/com.u8.sdk.U8UnityContext" },
    { "YoStarEN", "com.YoStarEN.Arknights/com.u8.sdk.U8UnityContext" },
    { "YoStarJP", "com.YoStarJP.Arknights/com.u8.sdk.U8UnityContext" },
    { "YoStarKR", "com.YoStarKR.Arknights/com.u8.sdk.U8UnityContext" },
    { "txwy", "tw.txwy.and.arknights/com.u8.sdk.U8UnityContext" }
};

int main(int argc, char* argv[])
{
    cxxopts::Options options(argv[0], "Maa utility tool for test purpose.");

    std::string adb = "adb";
    std::string adb_address = "127.0.0.1:5555";
    std::string client = "Official";

    if (getenv("MAA_ADB")) {
        adb = getenv("MAA_ADB");
    }

    if (getenv("MAA_ADB_SERIAL")) {
        adb_address = getenv("MAA_ADB_SERIAL");
    }

    if (getenv("MAA_CLIENT")) {
        client = getenv("MAA_CLIENT");
    }

    // clang-format off
    options.add_options()
        ("a,adb", "adb path, $MAA_ADB", cxxopts::value<std::string>()->default_value(adb))
        ("s,serial", "adb address, $MAA_ADB_SERIAL", cxxopts::value<std::string>()->default_value(adb_address))
        ("c,config", "config directory", cxxopts::value<std::string>()->default_value((std::filesystem::current_path() / "config").string()))
        ("t,client", "client, $MAA_CLIENT", cxxopts::value<std::string>()->default_value(client))
        ("h,help", "print usage", cxxopts::value<bool>())
        
        ("command", "command", cxxopts::value<std::string>()->default_value("help"))
        ("subcommand", "sub command", cxxopts::value<std::string>()->default_value("help"))
        ("params", "rest params", cxxopts::value<std::vector<std::string>>()->default_value(""))
        ;
    // clang-format on

    options.parse_positional({ "command", "subcommand", "params" });

    options.positional_help("[COMMAND] [SUBCOMMAND]");

    auto result = options.parse(argc, argv);

    auto cmd = result["command"].as<std::string>();

    if (cmd == "help" || result["help"].as<bool>()) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    using namespace MaaNS::ControllerNS;

    auto config = json::parse(read_adb_argv(result["config"].as<std::string>()));
    auto io = PlatformFactory::create();
    MaaSetGlobalOption(MaaGlobalOption_Logging, (std::filesystem::current_path() / "debug").string().c_str());

    Unit::UnitHelper::Argv::replacement adbRepl = { { "{ADB}", adb }, { "{ADB_SERIAL}", adb_address } };

    if (cmd == "connect") {
        auto connect = new Unit::Connection();
        connect->set_io(io);
        connect->parse(config.value());
        connect->set_replacement(adbRepl);

        std::cout << "return: " << std::boolalpha << connect->connect() << std::endl;
    }
    else if (cmd == "device_info") {
        auto device = new Unit::DeviceInfo();
        device->set_io(io);
        device->parse(config.value());
        device->set_replacement(adbRepl);

        std::cout << "uuid: " << device->request_uuid() << std::endl;
        std::cout << "resolution: " << device->request_resolution() << std::endl;
        std::cout << "orientation: " << device->request_orientation() << std::endl;
    }
    else if (cmd == "activity") {
        auto activity = new Unit::Activity();
        activity->set_io(io);
        activity->parse(config.value());
        activity->set_replacement(adbRepl);

        auto scmd = result["subcommand"].as<std::string>();

        if (scmd == "help") {
            std::cout << "Usage: " << argv[0] << " activity [start | stop]" << std::endl;
        }
        else if (scmd == "start") {
            std::cout << "return: " << std::boolalpha << activity->start(intents[client]) << std::endl;
        }
        else if (scmd == "stop") {
            std::cout << "return: " << std::boolalpha << activity->stop(intents[client]) << std::endl;
        }
    }
    else if (cmd == "tap_input") {
        auto tap = new Unit::TapInput();
        tap->set_io(io);
        tap->parse(config.value());
        tap->set_replacement(adbRepl);

        auto scmd = result["subcommand"].as<std::string>();
        auto params = result["params"].as<std::vector<std::string>>();

        if (scmd == "help") {
            std::cout << "Usage: " << argv[0] << " tap_input [click | swipe | press_key]" << std::endl;
        }
        else if (scmd == "click") {
            if (params.size() < 2) {
                std::cout << "Usage: " << argv[0] << " tap_input click [X] [Y]" << std::endl;
                return 0;
            }

            int x = atoi(params[0].c_str());
            int y = atoi(params[1].c_str());
            std::cout << "return: " << std::boolalpha << tap->click(x, y) << std::endl;
        }
        else if (scmd == "swipe") {
            if (params.size() < 5) {
                std::cout << "Usage: " << argv[0] << " tap_input swipe [X1] [Y1] [X2] [Y2] [DURATION]" << std::endl;
                return 0;
            }

            int x1 = atoi(params[0].c_str());
            int y1 = atoi(params[1].c_str());
            int x2 = atoi(params[2].c_str());
            int y2 = atoi(params[3].c_str());
            int dur = atoi(params[4].c_str());
            std::cout << "return: " << std::boolalpha << tap->swipe(x1, y1, x2, y2, dur) << std::endl;
        }
        else if (scmd == "press_key") {
            if (params.size() < 1) {
                std::cout << "Usage: " << argv[0] << " tap_input press_key [KEY]" << std::endl;
                return 0;
            }

            int key = atoi(params[0].c_str());
            std::cout << "return: " << std::boolalpha << tap->press_key(key) << std::endl;
        }
    }
    else if (cmd == "screencap") {
        auto device = new Unit::DeviceInfo();
        device->set_io(io);
        device->parse(config.value());
        device->set_replacement(adbRepl);

        auto res = device->request_resolution();

        auto scp = new Unit::Screencap();
        scp->set_io(io);
        scp->parse(config.value());
        scp->set_replacement(adbRepl);

        scp->init(res.value().width, res.value().height);

        auto scmd = result["subcommand"].as<std::string>();
        // auto params = result["params"].as<std::vector<std::string>>();

        auto now = std::chrono::steady_clock::now();

        if (scmd == "help") {
            std::cout << "Usage: " << argv[0]
                      << " screencap [test | raw_by_netcat | raw_with_gzip | encode | encode_to_file | netcat_address]"
                      << std::endl;
        }
        else if (scmd == "test") {
            std::cout << std::boolalpha << "return: " << scp->test_screencap(true) << std::endl;
        }
        else if (scmd == "raw_by_netcat") {
            auto mat = scp->screencap_raw_by_netcat();
            if (mat.has_value()) {
                cv::imwrite("temp.png", mat.value());
                std::cout << "image saved to temp.png" << std::endl;

                auto dur =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);
                std::cout << "time cost: " << dur << std::endl;
            }
        }
        else if (scmd == "raw_with_gzip") {
            auto mat = scp->screencap_raw_with_gzip();
            if (mat.has_value()) {
                cv::imwrite("temp.png", mat.value());
                std::cout << "image saved to temp.png" << std::endl;

                auto dur =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);
                std::cout << "time cost: " << dur << std::endl;
            }
        }
        else if (scmd == "encode") {
            auto mat = scp->screencap_encode();
            if (mat.has_value()) {
                cv::imwrite("temp.png", mat.value());
                std::cout << "image saved to temp.png" << std::endl;

                auto dur =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);
                std::cout << "time cost: " << dur << std::endl;
            }
        }
        else if (scmd == "encode_to_file") {
            auto mat = scp->screencap_encode_to_file();
            if (mat.has_value()) {
                cv::imwrite("temp.png", mat.value());
                std::cout << "image saved to temp.png" << std::endl;

                auto dur =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);
                std::cout << "time cost: " << dur << std::endl;
            }
        }
        else if (scmd == "netcat_address") {
            std::cout << scp->netcat_address() << std::endl;
        }
        scp->deinit();
    }
    else if (cmd == "invoke_app") {
        auto inv = new Unit::InvokeApp();
        inv->set_io(io);
        inv->parse(config.value());
        inv->set_replacement(adbRepl);

        std::ifstream f(".invokeapp");
        std::string invtp = "";
        if (f.is_open()) {
            f >> invtp;
            f.close();
        }

        inv->init(invtp);

        invtp = inv->get_tempname();

        auto scmd = result["subcommand"].as<std::string>();
        auto params = result["params"].as<std::vector<std::string>>();

        auto trackMinitouch = [](std::shared_ptr<MaaNS::ControllerNS::IOHandler> h) {
            while (true) {
                std::cout << "reading info..." << std::endl;
                auto str = h->read(2);
                if (str.empty()) {
#ifdef _WIN32
                    Sleep(2000);
#else
                    sleep(2);
#endif
                    continue;
                }
                auto pos = str.find('^');
                if (pos == std::string::npos) {
                    continue;
                }
                auto rpos = str.find('\n', pos);
                if (rpos == std::string::npos) {
                    continue;
                }
                auto info = str.substr(pos + 1, rpos - pos - 1);
                std::cout << "minitouch info: " << info << std::endl;
                break;
            }
            while (true) {
                std::string row;
                std::getline(std::cin, row);
                h->write(row + '\n');
            }
        };

        if (scmd == "help") {
            std::cout << "Usage: " << argv[0] << " invoke_app [abilist | push | chmod | invoke_bin]" << std::endl;
        }
        else if (scmd == "abilist") {
            std::cout << inv->abilist() << std::endl;
        }
        else if (scmd == "push") {
            if (params.size() < 1) {
                std::cout << "Usage: " << argv[0] << " invoke_app push [file]" << std::endl;
                return 0;
            }

            std::cout << "push as " << invtp << std::endl;
            std::cout << "return: " << std::boolalpha << inv->push(params[0]) << std::endl;
            std::ofstream fo(".invokeapp");
            fo << inv->get_tempname();
        }
        else if (scmd == "chmod") {
            std::cout << "chmod of " << invtp << std::endl;
            std::cout << "return: " << std::boolalpha << inv->chmod() << std::endl;
        }
        else if (scmd == "invoke_bin") {
            while (params.size() > 0 && params[0].empty()) {
                params.erase(params.begin());
            }
            std::cout << "params: " << params << std::endl;
            auto h = inv->invoke_bin(params.size() > 0 ? params[0] : "");
            trackMinitouch(h);
        }
        else if (scmd == "invoke_app") {
            if (params.size() < 1) {
                std::cout << "Usage: " << argv[0] << " invoke_app invoke_app [package]" << std::endl;
                return 0;
            }

            auto h = inv->invoke_app(params[0]);
            trackMinitouch(h);
        }
    }
}