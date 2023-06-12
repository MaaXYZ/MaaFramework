#include "utils.hpp"
#include <cstring>

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
    using namespace MaaNS::ControllerNS;

    auto repl = parse_argc(argc, argv);
    auto cfg = json::parse(read_adb_argv(std::filesystem::current_path().parent_path() / "test" / "config"));
    auto io = PlatformFactory::create();
    MaaSetGlobalOption(MaaGlobalOption_Logging, (std::filesystem::current_path() / "debug").string().c_str());

    auto connect = new Unit::Connection();
    connect->set_io(io);

    connect->parse(cfg.value());
    connect->set_replacement(repl);

    std::cout << connect->connect() << std::endl;

    auto activity = new Unit::Activity();
    activity->set_io(io);

    activity->parse(cfg.value());
    activity->set_replacement(repl);

    bool stop = false;
    std::string client = "Official";

    while (argc > 1) {
        const std::string arg = argv[1];
        if (arg == "--stop") {
            stop = true;
        }
        else if (arg.starts_with("--client=")) {
            client = arg.substr(9);
        }
        else {
            break;
        }
        argc--;
        argv++;
    }

    if (stop) {
        std::cout << "stoping " << client << ':' << intents[client] << std::endl;
        std::cout << activity->stop(intents[client]) << std::endl;
    }
    else {
        std::cout << "starting " << client << ':' << intents[client] << std::endl;
        std::cout << activity->start(intents[client]) << std::endl;
    }
}