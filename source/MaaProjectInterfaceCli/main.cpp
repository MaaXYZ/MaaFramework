#include <filesystem>
#include <iostream>

#include "MaaToolkit/MaaToolkitAPI.h"
#include "Utils/Runtime.h"

#include "interactor.h"

int main(int argc, char** argv)
{
    MaaToolkitInit();

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
