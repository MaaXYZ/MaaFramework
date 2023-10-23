#include <filesystem>

#include "module/PipelineSmoking.h"

#include "MaaFramework/MaaAPI.h"

int main([[maybe_unused]] int argc, char** argv)
{
    auto cur_dir = std::filesystem::path(argv[0]).parent_path();

    std::string logging_dir = (cur_dir / "debug").string();
    MaaSetGlobalOption(MaaGlobalOption_Logging, (void*)logging_dir.c_str(), logging_dir.size());
    bool on = true;
    MaaSetGlobalOption(MaaGlobalOption_DebugMode, &on, sizeof(on));

    bool ret = pipeline_smoking(cur_dir);

    return ret ? 0 : -1;
}
