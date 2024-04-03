#include <filesystem>
#include <iostream>

#include "MaaToolkit/MaaToolkitAPI.h"
#include "Utils/Runtime.h"
#include "Utils/Platform.h"

#include "interactor.h"

int main(int argc, char** argv)
{
    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    MaaToolkitInitOptionConfig(user_path.c_str(), "{}");

    Interactor interactor;

    if (!interactor.load(MAA_NS::library_dir())) {
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