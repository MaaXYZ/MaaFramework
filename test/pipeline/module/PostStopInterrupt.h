#pragma once

#include <filesystem>

// 回归用例：MaaXYZ/MaaFramework#1564
// post_stop 后 MaaContextRunTask 里的子任务阻塞点（wait_freezes / sleep）需在 2.5s 内返回
bool post_stop_interrupt(const std::filesystem::path& testset_dir);
