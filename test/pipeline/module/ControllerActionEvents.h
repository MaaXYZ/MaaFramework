#pragma once

// 回归用例：MaaXYZ/MaaFramework#1608
// 公共 post_* 动作每次都派发 Controller.Action.Starting / Succeeded，Tasker 内部动作不派发
bool controller_action_events();
