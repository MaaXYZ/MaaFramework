#pragma once

#include "ReplayRecording/ReplayController.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayController* create_replay_controller(const std::filesystem::path& recording_path);

MAA_CTRL_UNIT_NS_END
