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

using argv_t = char**;

inline MaaNS::ArgvWrapper<std::vector<std::string>>::replacement parse_argc(int argc, argv_t& argv)
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

    while (argc > 1) {
        const std::string arg = argv[1];
        if (arg.starts_with("--adb=")) {
            adb = arg.substr(6);
        }
        else if (arg.starts_with("--serial=")) {
            adb_address = arg.substr(9);
        }
        else {
            break;
        }
        argc--;
        argv++;
    }

    return { { "{ADB}", adb }, { "{ADB_SERIAL}", adb_address } };
}