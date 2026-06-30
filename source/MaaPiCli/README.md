# MaaPiCli

MaaFramework 官方提供的命令行 ProjectInterface Client，用于通过终端交互加载 `interface.json`、配置任务并运行。

## PI 协议支持版本

**已支持至 PI v2.5.0**（2026-03-23）

| PI 版本 | 状态 | 说明 |
|---------|------|------|
| v2.1.0 | ✅ | 基础协议 |
| v2.2.0 | ✅ | `attach_resource_path`、`import` |
| v2.3.0 | ✅ | `checkbox`、`option.controller/resource`、多级 option、`preset`、`import` 导入 preset |
| v2.3.1 | ✅ | Option 适用性过滤 |
| v2.4.0 | ✅ | `group` 任务分组、`task.group` |
| v2.5.0 | ✅ | Agent 子进程 `PI_*` 环境变量 |

## CLI 局限性

以下 PI 协议功能由于终端环境的固有限制，无法实现或仅提供降级支持：

### 无法实现

| 功能 | 涉及字段 | 原因 |
|------|----------|------|
| 图标显示 | 所有层级的 `icon` 字段（项目、控制器、资源、任务、选项、case、分组、预设） | CLI 无法显示图片 |
| Markdown 渲染 | `contact`、`license`、`welcome`、`description` 等支持 Markdown 的文本字段 | CLI 以纯文本原样输出，不渲染 Markdown 语法 |
| focus 通知渠道 | `focus.display` 的 `toast`、`notification`、`dialog`、`modal` 渠道 | CLI 不具备 toast/系统通知/弹窗能力 |
| 分组折叠 | `group.default_expand` | CLI 无折叠/展开概念，始终平铺显示 |
| 软件更新 | `github` 字段的版本检查与自动更新 | CLI 仅展示 GitHub 地址，不提供更新功能 |
| 资源分发 | `mirrorchyan_rid`、`mirrorchyan_multiplatform` | 资源包分发管理不属于 CLI 职责 |

### 降级实现

| 功能 | 协议行为 | CLI 实际行为 |
|------|----------|-------------|
| `welcome` 变更追踪 | Client 记录已展示内容，内容更新时重新弹窗 | 每次启动都展示，不追踪变更 |
| `focus` 回调消息 | Client 注册回调，按 `display` 渠道分发模板消息 | 未注册回调，不处理 focus 消息 |
| `group.description` | 显示分组的详细描述 | 仅显示分组名称/label |
| task 禁用态显示 | 不满足 resource/controller 约束的 task 灰显 | 直接过滤不显示 |
| option 禁用态显示 | 不满足约束的 option 灰显 | 直接跳过不提示 |

## 用法

```
MaaPiCli [全局参数] [子命令] [子命令参数]
```

### 全局参数

| 参数 | 说明 |
|------|------|
| `-h`, `--help` | 显示帮助信息 |
| `-v`, `--version` | 显示版本信息 |

### 子命令

| 子命令 | 说明 |
|--------|------|
| （无） | 交互模式 |
| `run [--task-config <path>] [--progress-level <0-2>]` | 运行任务（非交互），可加载任务配置文件、控制进度回显 |
| `list` | 列出可用的控制器、资源、任务和选项 |
| `generate-config pi [--controller <name>] [--adb-controller <pattern>] [--resource <name>] [--default-only] [--force]` | 生成 `config/maa_pi_config.json` |
| `generate-config task [--output <path>] [--default-only] [--force]` | 生成示例任务配置文件（TOML） |
| `help [command]` | 显示帮助信息，可指定子命令名 |

#### `run` 子命令选项

| 选项 | 说明 |
|------|------|
| `--task-config <path>` | 任务配置文件路径（TOML/JSON）；省略时直接运行已保存配置 |
| `--progress-level <N>` | 进度回显级别：`0`=静默，`1`=Task 级（默认），`2`=Task+Node 级 |

#### `generate-config pi` 子命令选项

| 选项 | 说明 |
|------|------|
| `--controller <name>` | 指定 controller 名称（默认取 `interface.json` 中第一个，通常是 Android） |
| `--adb-controller <pattern>` | ADB 设备过滤（按 name/path/address 子串匹配），仅在 controller 为 Adb 时生效 |
| `--resource <name>` | 指定 resource 名称（默认取第一个） |
| `--default-only` | 只写入 `default_check=true` 的任务 |
| `--force` | 文件已存在时强制覆盖（默认报错退出） |

#### `generate-config task` 子命令选项

| 选项 | 说明 |
|------|------|
| `--output, -o <path>` | 输出文件路径（默认 `sample_task_config.toml`） |
| `--default-only` | 只写入 `default_check=true` 的任务 |
| `--force` | 文件已存在时强制覆盖（默认报错退出） |

### 示例

```bash
# 交互模式
./MaaPiCli

# 直接运行已保存配置（非交互）
./MaaPiCli run

# 从任务配置文件直接运行（非交互）
./MaaPiCli run --task-config tasks.toml

# 同时打开 Task+Node 级进度回显
./MaaPiCli run --task-config tasks.toml --progress-level 2

# 完全静默运行
./MaaPiCli run --task-config tasks.toml --progress-level 0

# 列出可用信息
./MaaPiCli list

# 从 interface.json 的第一个 controller（通常是 Android）生成配置
./MaaPiCli generate-config pi

# 强制覆盖已存在的 maa_pi_config.json
./MaaPiCli generate-config pi --force

# 只写入 default_check=true 的任务
./MaaPiCli generate-config pi --default-only

# 指定 controller
./MaaPiCli generate-config pi --controller Android

# 过滤 ADB 设备（名称/路径/地址包含 MuMu 的第一个设备写入配置）
./MaaPiCli generate-config pi --adb-controller MuMu

# 同时指定 controller 和 ADB 过滤
./MaaPiCli generate-config pi --controller Android --adb-controller 127.0.0.1:16384

# 生成示例任务配置文件（所有任务）
./MaaPiCli generate-config task

# 只生成 default_check=true 的任务
./MaaPiCli generate-config task --default-only

# 显示 generate-config 子命令帮助
./MaaPiCli help generate-config
```

### run 子命令与任务配置文件

`run` 子命令读取 TOML 或 JSON 格式的任务配置文件，指定**本次要运行的任务列表**和**临时选项覆盖**。  
控制器、设备、资源等持久化配置会在启动时从 `maa_pi_config.json` 加载，无需额外传入 `-d`。

示例（TOML）：

```toml
# 至少需要一个 [[task]]

# 写法一：选项用 [[task.option]] 数组表展开
[[task]]
name = "TaskA"

[[task.option]]
name  = "difficulty"
value = "hard"

[[task]]
name = "TaskB"

# 写法二：选项用内联数组一行写完（适合选项较多时紧凑排列）
[[task]]
name = "任务一"
option = [
    { name = "选项1",          value = "30" },
]

# 全局选项覆盖（可选）
# [[global_option]]
# name  = "language"
# value = "en"
```

工作目录应为包含 `interface.json` 的项目根目录（即 MaaPiCli 可执行文件所在目录）。  
用户配置保存在 `config/maa_pi_config.json`。
