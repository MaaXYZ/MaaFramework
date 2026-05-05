# hook 维护说明

## 背景问题

Win32 `MessageInput` 在默认 `SendMessage` 模式下只向目标窗口发送鼠标消息，不会像 `WithCursorPos` 一样移动真实系统光标，也不会像 `WithWindowPos` 一样移动窗口去对齐光标。部分目标程序在处理 `WM_MOUSEMOVE`、`WM_LBUTTONDOWN`、`WM_MOUSEWHEEL` 等消息时会额外调用 `GetCursorPos` 获取当前光标屏幕坐标。  
如果此时真实鼠标停在别处，目标程序读到的坐标会和消息中的客户区坐标不一致，可能导致点击、拖动、滚轮等操作落点异常。  

本分支通过给目标进程注入一个小型 DLL，并 patch 目标进程里对 `user32!GetCursorPos` 的 IAT 调用，让它在自动化输入期间返回 Maa 侧写入的虚拟坐标。

## 文件改动

| 文件 | 类型 | 作用 |
| --- | --- | --- |
| `source/CMakeLists.txt` | 修改 | 在 `WITH_WIN32_CONTROLLER` 开启时新增 `MaaWin32GetCursorPosHook` 子目录。 |
| `source/MaaWin32ControlUnit/CMakeLists.txt` | 修改 | 让 `MaaWin32ControlUnit` 依赖 hook DLL，并在构建后把 DLL 复制到控制单元 DLL 同目录。 |
| `source/MaaWin32GetCursorPosHook/CMakeLists.txt` | 新增 | 定义 `MaaWin32GetCursorPosHook` 共享库及安装规则。 |
| `source/MaaWin32GetCursorPosHook/GetCursorPosHookDll.cpp` | 新增 | 目标进程内执行的 hook DLL，实现 IAT patch 和 `GetCursorPos` 替换逻辑。 |
| `source/MaaWin32ControlUnit/Input/GetCursorPosHookShared.h` | 新增 | 控制单元与 hook DLL 共用的共享内存结构、magic、version、映射名和 DLL 文件名。 |
| `source/MaaWin32ControlUnit/Input/GetCursorPosHook.h` | 新增 | 控制单元侧 hook 管理类声明。 |
| `source/MaaWin32ControlUnit/Input/GetCursorPosHook.cpp` | 新增 | 控制单元侧负责打开目标进程、创建共享内存、注入 DLL、写入/清除虚拟坐标。 |
| `source/MaaWin32ControlUnit/Input/MessageInput.h` | 修改 | 持有 `GetCursorPosHook`，并把 `prepare_mouse_position` 改为接收目标窗口句柄。 |
| `source/MaaWin32ControlUnit/Input/MessageInput.cpp` | 修改 | 在默认 `SendMessage` 且未启用 `WithCursorPos`/`WithWindowPos` 时启用 hook；收尾时清除虚拟坐标。 |

## 运行链路

| 步骤 | 控制单元侧 | 目标进程侧 |
| --- | --- | --- |
| 1 | `MessageInput::prepare_mouse_position(target, x, y)` 判断是否需要 hook。 | 无动作。 |
| 2 | `GetCursorPosHook::set_pos` 根据目标窗口找到目标进程 PID。 | 无动作。 |
| 3 | 控制单元创建 `Local\\MaaWin32GetCursorPosHook_<pid>` 文件映射，并映射 `SharedState`。 | 无动作。 |
| 4 | 如果 hook 尚未 ready，控制单元通过 `VirtualAllocEx`、`WriteProcessMemory`、`CreateRemoteThread(LoadLibraryW)` 注入 `MaaWin32GetCursorPosHook.dll`。 | `DllMain` 开线程执行初始化，避免在 loader lock 内做重操作。 |
| 5 | 控制单元轮询 `SharedState::hook_ready`，最多约 1 秒。 | DLL 打开同名共享内存，找到 `user32!GetCursorPos`，遍历当前模块并 patch IAT。 |
| 6 | 控制单元写入屏幕坐标 `x/y`，并把 `active` 置为 1。 | 被 patch 的 `GetCursorPos` 在 `active != 0` 时返回共享内存中的虚拟坐标。 |
| 7 | `touch_down`、`touch_move`、`scroll` 继续发送鼠标消息。 | 目标程序处理消息时读到虚拟光标位置。 |
| 8 | `finish_pos`、`restore_pos`、析构等收尾路径调用 `clear_get_cursor_pos_hook`。 | `active` 被置为 0 后，hook 函数回退到原始 `GetCursorPos`。 |

## 触发条件

| 条件 | 是否启用 hook | 原因 |
| --- | --- | --- |
| `Mode::SendMessage` 且未开启 `with_cursor_pos`、`with_window_pos` | 是 | 这是默认需要虚拟坐标补偿的场景。 |
| `Mode::SendMessage` 且开启 `with_cursor_pos` | 否 | 真实鼠标已经被移动到目标坐标。 |
| `Mode::SendMessage` 且开启 `with_window_pos` | 否 | 窗口会被移动到真实鼠标附近，已有独立追踪逻辑。 |
| `Mode::PostMessage` | 否 | `PostMessage` 是异步消息，当前 hook 激活窗口期不适合直接复用同步 `SendMessage` 的清理策略。 |

## 关键实现点

| 模块 | 实现点 |
| --- | --- |
| 共享状态 | `SharedState` 包含 `magic`、`version`、`hook_ready`、`active`、`x`、`y`、`last_error`，跨进程读写使用 `Interlocked*`。 |
| DLL 定位 | 控制单元通过自身模块路径推导同目录下的 `MaaWin32GetCursorPosHook.dll`，因此构建后复制和安装规则不能漏。 |
| 进程注入 | 控制单元使用远程线程调用 `LoadLibraryW`，注入前需要目标进程权限：`PROCESS_CREATE_THREAD`、`PROCESS_QUERY_LIMITED_INFORMATION`、`PROCESS_VM_OPERATION`、`PROCESS_VM_WRITE`。 |
| IAT patch | DLL 枚举当前进程模块，查找导入 `user32.dll` 的 `GetCursorPos` thunk，把函数地址替换为 `hooked_get_cursor_pos`。 |
| 坐标来源 | 控制单元写入的是 `client_to_screen(x, y)` 后的屏幕坐标，与 `GetCursorPos` 的 API 语义一致。 |
| 生命周期 | `MessageInput::finish_pos` 先清理 hook 的 `active` 状态，再处理光标/窗口恢复。`GetCursorPosHook` 析构时会清理共享内存映射和进程句柄。 |

## 维护注意事项

| 注意点 | 说明 |
| --- | --- |
| 架构位数必须一致 | hook DLL 需要被目标进程加载，32 位进程只能加载 32 位 DLL，64 位进程只能加载 64 位 DLL。 |
| 目标权限可能失败 | 跨权限级别、受保护进程、UAC 隔离或安全软件拦截时，`OpenProcess`、`CreateRemoteThread`、`LoadLibraryW` 可能失败。 |
| 只 patch 初始化时已有模块 | 当前 DLL 只在初始化时枚举并 patch 已加载模块；之后才加载、且也导入 `GetCursorPos` 的模块不会自动补 patch。 |
| 只覆盖 IAT 调用 | 直接通过 `GetProcAddress` 获取并调用 `GetCursorPos` 的代码不会被当前 IAT patch 覆盖。 |
| DLL 会留在目标进程中 | 当前实现只通过 `active` 控制是否返回虚拟坐标，没有卸载已注入 DLL，也没有还原 IAT。 |
| hook ready 依赖共享内存 | `hook_ready` 和 `last_error` 是控制单元判断注入状态的主要依据，排查问题时优先看相关日志。 |
| `PostMessage` 暂不启用 | 若未来要支持异步消息，需要重新设计 active 生命周期，避免消息尚未处理时虚拟坐标已被清除。 |
