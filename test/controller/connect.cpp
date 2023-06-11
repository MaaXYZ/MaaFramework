#include "Controller/Platform/PlatformFactory.h"
#include "Controller/Unit/Unit.h"
#include "MaaAPI.h"
#include <filesystem>
#include <gtest/gtest.h>
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

TEST(ConnectTest, StraightforwardTest)
{
    using namespace MaaNS::ControllerNS;

    MaaSetGlobalOption(MaaGlobalOption_Logging, (std::filesystem::current_path() / "debug").string().c_str());

    auto platformIO = PlatformFactory::create();
    auto connect = new Unit::Connection(platformIO);

    auto cfg = json::parse(read_adb_argv(std::filesystem::current_path().parent_path() / "test" / "config"));
    EXPECT_TRUE(cfg.has_value());

    connect->parse(cfg.value());
    connect->set_replacement({ { "{ADB}", "adb" }, { "{ADB_SERIAL}", "127.0.0.1:16384" } });
    EXPECT_TRUE(connect->connect());

    // EXPECT_TRUE(connect->kill_server());
}