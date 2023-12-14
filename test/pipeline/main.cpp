#include <filesystem>

#include "module/PipelineSmoking.h"
#include "module/RunWithoutFile.h"

#include "MaaFramework/MaaAPI.h"

int main([[maybe_unused]] int argc, char** argv)
{
    auto cur_dir = std::filesystem::path(argv[0]).parent_path();
    auto testset_dir = cur_dir.parent_path() / "test";
    if (argc == 2) {
        testset_dir = argv[1];
    }

    std::string logging_dir = (cur_dir / "debug").string();
    MaaSetGlobalOption(MaaGlobalOption_LogDir, static_cast<void*>(logging_dir.data()), logging_dir.size());
    bool on = true;
    MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &on, sizeof(on));
    MaaLoggingLevel lv = MaaLoggingLevel_Info;
    MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &lv, sizeof(lv));

    if (!run_without_file(testset_dir)) {
        return -1;
    }
    if (!pipeline_smoking(testset_dir)) {
        return -1;
    }

    return 0;
}
