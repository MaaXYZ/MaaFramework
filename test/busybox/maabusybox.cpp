#define _CRT_SECURE_NO_WARNINGS

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

#include <cxxopts/cxxopts.hpp>

#include "MaaAPI.h"
#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaUtils/Logger.hpp"
#include "Utils/NoWarningCV.h"

std::ostream& operator<<(std::ostream& os, const MAA_CTRL_UNIT_NS::DeviceResolution& res);

std::string read_controller_config(const std::string& cur_dir);
double test_screencap(std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapAPI> scp, int count = 10);

bool demo_waiting(const std::filesystem::path& cur_dir, const std::string& adb, const std::string& adb_address,
                  const std::string& adb_config, const std::string& task);

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
        ("c,config", "config directory", cxxopts::value<std::string>()->default_value(std::filesystem::current_path().string()))
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

    adb = result["adb"].as<std::string>();
    adb_address = result["serial"].as<std::string>();
    client = result["client"].as<std::string>();

    std::string config = read_controller_config(result["config"].as<std::string>());

    using namespace MAA_CTRL_UNIT_NS;

    if (cmd == "task") {
        auto task = result["subcommand"].as<std::string>();
        LogInfo << demo_waiting(std::filesystem::current_path(), adb, adb_address, config, task);
    }
    else if (cmd == "connect") {
        auto connection = create_adb_connection(adb.c_str(), adb_address.c_str(), 0, config.c_str());
        if (!connection) {
            LogError << "Failed to create connection";
            return -1;
        }
        LogInfo << "connect: " << connection->connect();
    }
    else if (cmd == "device_info") {
        auto device_info = create_adb_device_info(adb.c_str(), adb_address.c_str(), 0, config.c_str());

        if (!device_info) {
            LogError << "Failed to create device_info";
            return -1;
        }

        LogInfo << "uuid:" << device_info->request_uuid();
        LogInfo << "resolution:" << device_info->request_resolution().has_value();
        LogInfo << "orientation:" << device_info->request_orientation();
    }
    else if (cmd == "activity") {
        auto activity = create_adb_activity(adb.c_str(), adb_address.c_str(), 0, config.c_str());
        if (!activity) {
            LogError << "Failed to create activity";
            return -1;
        }

        static const std::map<std::string_view, std::string> kAppIntents = {
            { "Official", "com.hypergryph.arknights/com.u8.sdk.U8UnityContext" },
            { "Bilibili", "com.hypergryph.arknights.bilibili/com.u8.sdk.U8UnityContext" },
            { "YoStarEN", "com.YoStarEN.Arknights/com.u8.sdk.U8UnityContext" },
            { "YoStarJP", "com.YoStarJP.Arknights/com.u8.sdk.U8UnityContext" },
            { "YoStarKR", "com.YoStarKR.Arknights/com.u8.sdk.U8UnityContext" },
            { "txwy", "tw.txwy.and.arknights/com.u8.sdk.U8UnityContext" }
        };

        auto scmd = result["subcommand"].as<std::string>();

        if (scmd == "start") {
            LogInfo << "start" << activity->start(kAppIntents.at(client));
        }
        else if (scmd == "stop") {
            LogInfo << "stop" << activity->stop(kAppIntents.at(client));
        }
        else {
            LogInfo << "Usage: " << argv[0] << " activity [start | stop]";
        }
    }
    else if (cmd == "touch") {
        auto scmd = result["subcommand"].as<std::string>();

        if (scmd == "help") {
            std::cout << "Usage: " << argv[0] << " touch [adb | minitouch | maatouch]";
            return 0;
        }

        MaaAdbControllerType type = -1;

        if (scmd == "adb") {
            type = MaaAdbControllerType_Touch_Adb;
        }
        else if (scmd == "minitouch") {
            type = MaaAdbControllerType_Touch_MiniTouch;
        }
        else if (scmd == "maatouch") {
            type = MaaAdbControllerType_Touch_MaaTouch;
        }
        else {
            return 1;
        }

        auto input = create_adb_touch_input(adb.c_str(), adb_address.c_str(), type, config.c_str());
        std::cout << "Usage:\n"
                  << "click: c [x] [y]\n"
                  << "swipe: s [x1] [y1] [x2] [y2] [d]\n"
                  << "quit: q\n"
                  << std::flush;

        bool quit = false;

        while (!quit) {
            std::string in;
            std::getline(std::cin, in);
            std::istringstream is(in);

            char c = static_cast<char>(is.get());
            switch (c) {
            case 'c': {
                int x = 0, y = 0;
                if (!is >> x >> y) {
                    break;
                }
                std::cout << "click: " << input->click(x, y) << std::endl;
                break;
            }
            case 's': {
                int x1 = 0, y1 = 0, x2 = 0, y2 = 0, d = 0;
                if (!is >> x1 >> y1 >> x2 >> y2 >> d) {
                    break;
                }
                std::vector<MAA_CTRL_UNIT_NS::SwipeStep> steps { { x1, y1, d }, { x2, y2, 0 } };
                std::cout << "swipe: " << input->swipe(steps);
                break;
            }
            case 'q': {
                quit = true;
                break;
            }
            }
        }
    }
    // else if (cmd == "tap_input") {
    //     auto tap = initUnit(new Unit::TapInput);

    //     auto scmd = result["subcommand"].as<std::string>();
    //     auto params = result["params"].as<std::vector<std::string>>();

    //     if (scmd == "help") {
    //         std::cout << "Usage: " << argv[0] << " tap_input [click | swipe | press_key]" << std::endl;
    //     }
    //     else if (scmd == "click") {
    //         if (params.size() < 2) {
    //             std::cout << "Usage: " << argv[0] << " tap_input click [X] [Y]" << std::endl;
    //             return 0;
    //         }

    //         int x = atoi(params[0].c_str());
    //         int y = atoi(params[1].c_str());
    //         std::cout << "return: " << std::boolalpha << tap->click(x, y) << std::endl;
    //     }
    //     else if (scmd == "swipe") {
    //         if (params.size() < 5) {
    //             std::cout << "Usage: " << argv[0] << " tap_input swipe [X1] [Y1] [X2] [Y2] [DURATION]" << std::endl;
    //             return 0;
    //         }

    //         int x1 = atoi(params[0].c_str());
    //         int y1 = atoi(params[1].c_str());
    //         int x2 = atoi(params[2].c_str());
    //         int y2 = atoi(params[3].c_str());
    //         int dur = atoi(params[4].c_str());
    //         std::cout << "return: " << std::boolalpha << tap->swipe(x1, y1, x2, y2, dur) << std::endl;
    //     }
    //     else if (scmd == "press_key") {
    //         if (params.size() < 1) {
    //             std::cout << "Usage: " << argv[0] << " tap_input press_key [KEY]" << std::endl;
    //             return 0;
    //         }

    //         int key = atoi(params[0].c_str());
    //         std::cout << "return: " << std::boolalpha << tap->press_key(key) << std::endl;
    //     }
    // }
    else if (cmd == "screencap") {
        auto device = create_adb_device_info(adb.c_str(), adb_address.c_str(), 0, config.c_str());
        if (!device) {
            LogError << "Failed to create device_info";
            return -1;
        }

        auto res = device->request_resolution();
        if (!res) {
            LogError << "Failed to get resolution";
            return -1;
        }

        auto scmd = result["subcommand"].as<std::string>();
        // auto params = result["params"].as<std::vector<std::string>>();

        if (scmd == "help") {
            LogInfo << "Usage: " << argv[0]
                    << " screencap [profile | fastest | raw_by_netcat | raw_with_gzip | encode | encode_to_file | "
                       "minicap_direct | minicap_strean]";
            return 0;
        }

        bool profile = false;
        std::map<std::string, double> cost;

        auto create_scp = [&adb, &adb_address, &config, &res](MaaAdbControllerType type) {
            auto scp = create_adb_screencap(adb.c_str(), adb_address.c_str(), type, config.c_str());
            scp->init(res->width, res->height);
            return scp;
        };

        if (scmd == "profile") {
            profile = true;
        }

#define TEST_SCP(type, id)                                          \
    if (profile || scmd == type) {                                  \
        auto scp = create_scp(MaaAdbControllerType_Screencap_##id); \
        test_screencap(scp);                                        \
    }

        TEST_SCP("fastest", FastestWay)
        TEST_SCP("raw_by_netcat", RawByNetcat)
        TEST_SCP("raw_with_gzip", RawWithGzip)
        TEST_SCP("encode", Encode)
        TEST_SCP("encode_to_file", EncodeToFile)
        TEST_SCP("minicap_direct", MinicapDirect)
        TEST_SCP("minicap_stream", MinicapStream)
    }
    // else if (cmd == "invoke_app") {
    //     auto inv = initUnit(new Unit::InvokeApp);

    //     std::ifstream f(".invokeapp");
    //     std::string invtp = "";
    //     if (f.is_open()) {
    //         f >> invtp;
    //         f.close();
    //     }

    //     inv->init(invtp);

    //     invtp = inv->get_tempname();

    //     auto scmd = result["subcommand"].as<std::string>();
    //     auto params = result["params"].as<std::vector<std::string>>();

    //     auto trackMinitouch = [](std::shared_ptr<MaaNS::ControllerNS::IOHandler> h) {
    //         while (true) {
    //             std::cout << "reading info..." << std::endl;
    //             auto str = h->read(2);
    //             if (str.empty()) {
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //                 continue;
    //             }
    //             auto pos = str.find('^');
    //             if (pos == std::string::npos) {
    //                 continue;
    //             }
    //             auto rpos = str.find('\n', pos);
    //             if (rpos == std::string::npos) {
    //                 continue;
    //             }
    //             auto info = str.substr(pos + 1, rpos - pos - 1);
    //             std::cout << "minitouch info: " << info << std::endl;
    //             break;
    //         }
    //         while (true) {
    //             std::string row;
    //             std::getline(std::cin, row);
    //             h->write(row + '\n');
    //         }
    //     };

    //     if (scmd == "help") {
    //         std::cout << "Usage: " << argv[0] << " invoke_app [abilist | push | chmod | invoke_bin]" << std::endl;
    //     }
    //     else if (scmd == "abilist") {
    //         std::cout << inv->abilist() << std::endl;
    //     }
    //     else if (scmd == "push") {
    //         if (params.size() < 1) {
    //             std::cout << "Usage: " << argv[0] << " invoke_app push [file]" << std::endl;
    //             return 0;
    //         }

    //         std::cout << "push as " << invtp << std::endl;
    //         std::cout << "return: " << std::boolalpha << inv->push(params[0]) << std::endl;
    //         std::ofstream fo(".invokeapp");
    //         fo << inv->get_tempname();
    //     }
    //     else if (scmd == "chmod") {
    //         std::cout << "chmod of " << invtp << std::endl;
    //         std::cout << "return: " << std::boolalpha << inv->chmod() << std::endl;
    //     }
    //     else if (scmd == "invoke_bin") {
    //         while (params.size() > 0 && params[0].empty()) {
    //             params.erase(params.begin());
    //         }
    //         std::cout << "params: " << params << std::endl;
    //         auto h = inv->invoke_bin(params.size() > 0 ? params[0] : "");
    //         trackMinitouch(h);
    //     }
    //     else if (scmd == "invoke_app") {
    //         if (params.size() < 1) {
    //             std::cout << "Usage: " << argv[0] << " invoke_app invoke_app [package]" << std::endl;
    //             return 0;
    //         }

    //         auto h = inv->invoke_app(params[0]);
    //         trackMinitouch(h);
    //     }
    // }

    return 0;
}

std::string read_controller_config(const std::string& cur_dir)
{
    std::ifstream ifs(std::filesystem::path(cur_dir) / "controller_config.json", std::ios::in);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open controller_config.json\n"
                  << "Please copy controller_config.json to " << std::filesystem::path(cur_dir) << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

std::ostream& operator<<(std::ostream& os, const MAA_CTRL_UNIT_NS::DeviceResolution& res)
{
    return os << "{ width: " << res.width << ", height: " << res.height << " }";
}

double test_screencap(std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapAPI> scp, int count)
{
    std::chrono::milliseconds sum(0);
    for (int i = 0; i < count; i++) {
        auto now = std::chrono::steady_clock::now();
        auto mat = scp->screencap();
        if (mat.has_value()) {
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);

            auto file = std::format("temp-{}.png", i);
            cv::imwrite(file, mat.value());
            std::cout << "image saved to " << file << std::endl;

            std::cout << "time cost: " << dur.count() << "ms" << std::endl;
            sum += dur;
        }
    }
    double cost = sum.count() / double(count);
    std::cout << "average time cost: " << cost << "ms" << std::endl;
    return cost;
}

bool demo_waiting(const std::filesystem::path& cur_dir, const std::string& adb, const std::string& adb_address,
                  const std::string& adb_config, const std::string& task)
{
    auto maa_handle = MaaCreate(nullptr, nullptr);
    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    auto controller_handle =
        MaaAdbControllerCreate(adb.c_str(), adb_address.c_str(),
                               MaaAdbControllerType_Input_Preset_Minitouch | MaaAdbControllerType_Screencap_FastestWay,
                               adb_config.c_str(), nullptr, nullptr);

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    auto resource_id = MaaResourcePostResource(resource_handle, (cur_dir / "resource").string().c_str());
    auto connection_id = MaaControllerPostConnection(controller_handle);

    MaaResourceWait(resource_handle, resource_id);
    MaaControllerWait(controller_handle, connection_id);

    auto destroy = [&]() {
        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    };

    if (!MaaInited(maa_handle)) {
        destroy();
        return false;
    }

    auto task_id = MaaPostTask(maa_handle, task.c_str(), MaaTaskParam_Empty);
    MaaTaskWait(maa_handle, task_id);

    destroy();

    return true;
}
