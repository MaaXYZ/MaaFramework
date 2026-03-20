#include "ReplayControllerMgr.h"

#include "MaaUtils/Logger.h"
#include "ReplayRecording/RecordParser.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayController* create_replay_controller(const std::filesystem::path& path)
{
    auto record_opt = RecordParser::parse(path);
    if (!record_opt) {
        LogError << "Failed to parse record file:" << path;
        return nullptr;
    }
    return new ReplayController(std::move(*record_opt));
}

MAA_CTRL_UNIT_NS_END
