#include <filesystem>

#include "Configurator.h"
#include "Utils/Platform.h"

int main(int argc, char** argv)
{
    auto app_path = MaaNS::path(argv[0]);
    auto project_dir = app_path.parent_path();

    Configurator config;
    config.load(project_dir);

    return 0;
}
