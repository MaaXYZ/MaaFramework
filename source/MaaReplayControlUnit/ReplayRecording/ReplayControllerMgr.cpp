#include "ReplayControllerMgr.h"

#include "MaaUtils/Logger.h"
#include "ReplayRecording/RecordParser.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayController* create_replay_controller(const std::filesystem::path& dump_dir)
{
    auto recording_path = dump_dir / "recording.jsonl";
    auto record_opt = RecordParser::parse(recording_path);
    if (!record_opt) {
        LogError << "Failed to parse record file:" << recording_path;
        return nullptr;
    }
    return new ReplayController(std::move(*record_opt));
}

MAA_CTRL_UNIT_NS_END
