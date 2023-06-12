#include "Controller/Platform/PlatformFactory.h"
#include "Controller/Unit/ControlUnit.h"
#include "MaaAPI.h"
#include "Utils/ArgvWrapper.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>

inline std::string read_adb_argv(const std::filesystem::path& cur_dir)
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

inline MaaNS::ArgvWrapper<std::vector<std::string>>::replacement parse_argc(int argc, char* argv[])
{
    std::string adb = "adb";
    std::string adb_address = "127.0.0.1:5555";

    auto eadb = getenv("MAA_ADB");
    if (eadb) {
        adb = eadb;
    }

    auto eadb_address = getenv("MAA_ADB_SERIAL");
    if (eadb_address) {
        adb_address = eadb_address;
    }

    if (argc > 1) {
        adb = argv[1];
    }
    if (argc > 2) {
        adb_address = argv[2];
    }

    return { { "{ADB}", adb }, { "{ADB_SERIAL}", adb_address } };
}