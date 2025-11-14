#include "ReplayRecordingMgr.h"

#include "MaaUtils/Logger.h"
#include "ReplayRecording/RecordParser.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayRecording* create_replay_recording(const std::filesystem::path& path)
{
    auto record_opt = RecordParser::parse(path);
    if (!record_opt) {
        LogError << "Failed to parse record file:" << path;
        return nullptr;
    }
    return new ReplayRecording(std::move(*record_opt));
}

MAA_CTRL_UNIT_NS_END
