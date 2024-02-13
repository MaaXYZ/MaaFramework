#include <filesystem>
#include <iostream>

#include "Utils/Platform.h"

#include "interactor.h"
#include "runner.h"

void mpause()
{
    std::cout << "\nPress Enter to continue...";
    std::cin.get();
}

int main(int argc, char** argv)
{
    std::ignore = argc;

    auto app_path = MaaNS::path(argv[0]);
    auto project_dir = app_path.parent_path();

    Interactor interactor;
    if (!interactor.load(project_dir)) {
        mpause();
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
    }
    else {
        interactor.interact();
    }

    auto runtime_opt = interactor.generate_runtime();
    if (!runtime_opt) {
        mpause();
        return -1;
    }

    bool ret = Runner::run(*runtime_opt);
    if (!ret) {
        mpause();
        return -1;
    }

    mpause();
    return 0;
}
