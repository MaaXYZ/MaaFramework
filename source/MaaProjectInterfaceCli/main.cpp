#include <filesystem>

#include "Utils/Platform.h"

#include "interactor.h"

int main(int argc, char** argv)
{
    std::ignore = argc;

    auto app_path = MaaNS::path(argv[0]);
    auto project_dir = app_path.parent_path();

    Interactor interactor;
    if (!interactor.load(project_dir)) {
        return -1;
    }

    interactor.interact();

    return 0;
}
