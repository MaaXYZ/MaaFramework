#include "ReplayRecordingMgr.h"

#include "ReplayRecording/RecordParser.h"
#include "Utils/Logger.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

std::shared_ptr<ReplayRecording> create_replay_recording(const std::filesystem::path& path)
{
    auto record_opt = RecordParser::parse(path);
    if (!record_opt) {
        LogError << "Failed to parse record file:" << path;
        return nullptr;
    }
    return std::make_shared<ReplayRecording>(std::move(*record_opt));
}

MAA_DBG_CTRL_UNIT_NS_END
