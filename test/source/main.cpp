#include <filesystem>

#include "module/PipelineSmoking.h"

#include "MaaFramework/MaaAPI.h"

int main([[maybe_unused]] int argc, char** argv)
{
    auto cur_dir = std::filesystem::path(argv[0]).parent_path();
    auto testset_dir = cur_dir.parent_path() / "test";
    if (argc == 2) {
        testset_dir = argv[1];
    }

    std::string logging_dir = (cur_dir / "debug").string();
    MaaSetGlobalOption(MaaGlobalOption_LogDir, (void*)logging_dir.c_str(), logging_dir.size());
    bool on = true;
    MaaSetGlobalOption(MaaGlobalOption_DebugMode, &on, sizeof(on));
    MaaLoggingLevel lv = MaaLoggingLevel_All;
    MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &lv, sizeof(lv));

    bool ret = pipeline_smoking(testset_dir);

    return ret ? 0 : -1;
}
