#include <fstream>
#include <iostream>
#include <meojson/json.hpp>
#include <sstream>
#include <string>

#include "MaaFramework/MaaAPI.h"
#include "MaaPI/MaaPIAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

#include "interactor.h"

int main(int argc, char* argv[])
{
    auto root = MAA_NS::library_dir();

    if (argc > 1) {
        root = argv[1];
    }

    std::string user_path = MAA_NS::path_to_utf8_string(root);

    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    Interactor interactor;

    if (!interactor.load(root)) {
        return -1;
    }

    bool direct = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) != "-d") {
            continue;
        }
        direct = true;
        break;
    }

    if (direct) {
        interactor.print_config();
        bool ret = interactor.run();

        return ret ? 0 : -1;
    }

    interactor.interact();
    return 0;
}
