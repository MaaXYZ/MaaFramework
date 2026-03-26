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

```bash
# 交互模式
./MaaPiCli

# 直接运行（使用已保存的配置）
./MaaPiCli -d
```

工作目录应为包含 `interface.json` 的项目根目录（即 MaaPiCli 可执行文件所在目录）。  
用户配置保存在 `config/maa_pi_config.json`。
