#include "Controller/Platform/PlatformFactory.h"
#include "Controller/Unit/ControlUnit.h"
#include "MaaAPI.h"
#include <filesystem>
#include <iostream>

std::string read_adb_argv(const std::filesystem::path& cur_dir)
{
    std::ifstream ifs(cur_dir / "adb_argv.json", std::ios::in);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open adb_argv.json\n"
                  << "Please copy sample/cpp/config/adb_argv.json to " << cur_dir << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

int main()
{
    using namespace MaaNS::ControllerNS;

    auto cfg = json::parse(read_adb_argv(std::filesystem::current_path().parent_path() / "test" / "config"));
    MaaSetGlobalOption(MaaGlobalOption_Logging, (std::filesystem::current_path() / "debug").string().c_str());

    auto platformIO = PlatformFactory::create();
    auto connect = new Unit::Connection();
    connect->set_io(platformIO);

    connect->parse(cfg.value());
    connect->set_replacement({ { "{ADB}", "adb" }, { "{ADB_SERIAL}", "127.0.0.1:16384" } });

    std::cout << connect->connect() << std::endl;
}