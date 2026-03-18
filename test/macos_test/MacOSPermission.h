
// 检查屏幕录制权限
// 已授权则返回 true，否则返回 false
bool check_screen_recording_permission(void);

// 请求屏幕录制权限（在 macOS 10.15+ 会弹出系统提示）
// 授权则返回 true，否则返回 false
bool request_screen_recording_permission(void);

// 打开“屏幕录制”隐私设置页，方便用户手动授予权限
// 返回 true 如果成功打开设置页
bool open_screen_recording_privacy_settings(void);

// 检查辅助功能（Accessibility）权限
// 已授权则返回 true，否则返回 false
bool check_accessibility_permission(void);

// 请求辅助功能权限（在支持的平台上会弹出系统提示 / 打开系统设置）
// 如果已经被信任或用户已授权则返回 true，否则返回 false
bool request_accessibility_permission(void);

// 打开“辅助功能”隐私设置页，方便用户手动授予权限
// 返回 true 如果成功打开设置页
bool open_accessibility_privacy_settings(void);
