#pragma once

#include "ReplayRecording/ReplayRecording.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayRecording* create_replay_recording(const std::filesystem::path& path);

MAA_CTRL_UNIT_NS_END
