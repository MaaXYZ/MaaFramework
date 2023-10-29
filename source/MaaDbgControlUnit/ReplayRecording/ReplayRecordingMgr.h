#pragma once

#include "ReplayRecording/ReplayRecording.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

std::shared_ptr<ReplayRecording> create_replay_recording(const std::filesystem::path& path);

MAA_DBG_CTRL_UNIT_NS_END
