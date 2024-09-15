#include "Utils/Platform.h"
#include "Utils/Runtime.h"

#include "MaaToolkit/ProjectInterface/MaaToolkitProjectInterface.h"

int main(int argc, char** argv)
{
    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());
    std::string resource_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    bool directly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) != "-d") {
            continue;
        }
        directly = true;
        break;
    }

    bool ret = MaaToolkitProjectInterfaceRunCli(0, resource_path.c_str(), user_path.c_str(), directly, nullptr, nullptr);
    return ret ? 0 : -1;
}
