# MaaFramework 项目指南

本文档旨在帮助开发者（包括 AI）快速了解 MaaFramework 项目结构，以便参与框架本身的开发。

> [!WARNING]
> 本文档面向希望开发 MaaFramework 本身的贡献者，而非集成使用者。集成使用请参考 [快速开始](docs/zh_cn/1.1-快速开始.md)。

## 项目概述

**MaaFramework** 是一个基于图像识别技术的自动化黑盒测试框架，使用 C++20 编写，提供 C 语言 API 供各语言绑定调用。既支持通过 JSON 配置任务流水线（Pipeline）的低代码开发，也提供完整的集成接口供深度定制。

## 目录结构

```text
MaaFramework/
├── include/                    # C 语言头文件（公开 API）
│   ├── MaaFramework/           # 核心框架 API
│   │   ├── MaaDef.h            # 类型定义、枚举、回调签名
│   │   ├── MaaMsg.h            # 回调消息定义
│   │   ├── MaaAPI.h            # 主入口头文件
│   │   ├── Instance/           # 实例相关 API
│   │   │   ├── MaaTasker.h     # 任务执行器
│   │   │   ├── MaaResource.h   # 资源管理器
│   │   │   ├── MaaController.h # 控制器
│   │   │   └── MaaContext.h    # 任务上下文
│   │   ├── Global/             # 全局配置
│   │   └── Utility/            # 工具类（Buffer 等）
│   ├── MaaToolkit/             # 工具包 API（设备发现等）
│   ├── MaaAgentServer/         # Agent 服务端 API
│   └── MaaAgentClient/         # Agent 客户端 API
├── source/                     # 源代码
│   ├── MaaFramework/           # 核心框架实现
│   │   ├── API/                # C API 实现
│   │   ├── Task/               # 任务执行逻辑
│   │   ├── Vision/             # 图像识别算法
│   │   ├── Resource/           # 资源解析（Pipeline）
│   │   ├── Controller/         # 控制器实现
│   │   └── Tasker/             # 任务调度器
│   ├── MaaToolkit/             # 工具包实现
│   ├── MaaAgentServer/         # Agent 服务端
│   ├── MaaAgentClient/         # Agent 客户端
│   └── binding/                # 语言绑定
│       ├── Python/             # Python 绑定
│       └── NodeJS/             # Node.js 绑定
├── docs/                       # 文档
│   ├── zh_cn/                  # 中文文档
│   └── en_us/                  # 英文文档
├── sample/                     # 示例代码
│   ├── cpp/                    # C++ 示例
│   ├── python/                 # Python 示例
│   ├── nodejs/                 # Node.js 示例
│   └── resource/               # 示例资源（Pipeline JSON）
├── tools/                      # 开发工具
│   ├── pipeline.schema.json    # Pipeline JSON Schema
│   └── interface.schema.json   # ProjectInterface Schema
└── test/                       # 测试代码
```

## 核心概念

### 术语定义

| 术语 | 说明 |
|------|------|
| **Node（节点）** | Pipeline JSON 中的一个完整对象，包含识别、动作、后继节点等配置 |
| **Task（任务）** | 若干 Node 按顺序相连的逻辑流程 |
| **Entry（入口）** | Task 中的第一个 Node |
| **Pipeline（流水线）** | pipeline 文件夹中所有 Node 的全体 |
| **Bundle** | 标准资源目录结构，包含 pipeline、model、image 等文件夹 |
| **Resource（资源）** | 多个 Bundle 按顺序加载后的资源结构 |
| **Agent** | 将 MaaFW 本体与 Custom 逻辑分离至独立进程的机制 |

### 核心对象

1. **MaaResource**：资源管理器，加载 Pipeline JSON、模型、图像模板
2. **MaaController**：控制器，负责截图和输入操作（ADB/Win32/PlayCover）
3. **MaaTasker**：任务执行器，绑定 Resource 和 Controller 后执行任务
4. **MaaContext**：任务上下文，在 Custom 回调中提供运行时操作能力

### 执行流程

```text
1. 创建 Resource → 加载资源
2. 创建 Controller → 连接设备
3. 创建 Tasker → 绑定 Resource 和 Controller
4. post_task(entry) → 执行任务流水线
```

## Pipeline 协议

### 识别算法类型

| 类型 | 说明 | 关键参数 |
|------|------|----------|
| `DirectHit` | 直接命中，不进行识别 | - |
| `TemplateMatch` | 模板匹配（找图） | `template`, `threshold`, `roi` |
| `FeatureMatch` | 特征匹配（抗透视） | `template`, `count`, `detector` |
| `ColorMatch` | 颜色匹配（找色） | `lower`, `upper`, `method` |
| `OCR` | 文字识别 | `expected`, `model` |
| `NeuralNetworkClassify` | 深度学习分类 | `model`, `expected` |
| `NeuralNetworkDetect` | 深度学习目标检测 | `model`, `expected` |
| `Custom` | 自定义识别 | `custom_recognition` |

### 动作类型

| 类型 | 说明 | 关键参数 |
|------|------|----------|
| `DoNothing` | 不执行任何操作 | - |
| `Click` | 点击 | `target`, `target_offset` |
| `LongPress` | 长按 | `target`, `duration` |
| `Swipe` | 滑动 | `begin`, `end`, `duration` |
| `MultiSwipe` | 多指滑动 | `swipes` |
| `ClickKey` | 按键 | `key` |
| `InputText` | 输入文本 | `input_text` |
| `StartApp` / `StopApp` | 启动/停止应用 | `package` |
| `Command` | 执行外部命令 | `exec`, `args` |
| `Shell` | 执行 ADB Shell | `cmd` |
| `Custom` | 自定义动作 | `custom_action` |

### 节点属性

支持在 `next` 列表中使用节点属性，如有需要请阅读 [节点属性章节](docs/zh_cn/3.1-任务流水线协议.md#节点属性)。

## 开发规范

### 代码风格

- 优先使用 C++20 特性，如 `std::format`、`std::ranges`、`std::views` 等
- 当 Boost 提供比标准库显著更优的实现，且不引入额外依赖时，考虑使用 Boost（如 `boost::wregex` 优于 `std::regex`）
- 主要遵循 Google C++ 代码风格

#### 函数拆分与 lambda 使用

- **避免长 lambda**：lambda 适合做轻量的适配/回调（通常几行内即可读懂）。当 lambda 内包含较长的 `switch`/循环/复杂控制流或超过可读的长度时，应提取为具名函数/成员方法，提升可读性、可测试性与复用性。

#### 控制流与嵌套

采用 **Guard Clauses（守卫语句）** 模式，通过前置条件检查和早返回降低代码嵌套层级：

- **前置条件优先检查**：在函数入口处验证参数有效性、资源可用性等前置条件，不满足时立即返回，避免将主逻辑包裹在深层条件块中
- **异常路径早返回**：错误处理、边界情况应尽早处理并返回，使主逻辑保持在最外层
- **避免 else 分支嵌套**：当 if 分支以 return 结束时，后续逻辑无需包裹在 else 中

#### 异常使用原则

- **尽量不使用 `try/catch` 作为常规控制流**：优先通过前置条件检查来规避无效调用路径（参数/状态/资源/外部依赖不满足则不调用），并以返回值/状态码/可查询状态的方式显式失败
- **仅在边界集中兜底**：确有第三方库可能抛异常时，将 `try/catch` 收敛在模块边界/线程入口/任务调度入口等少数位置，统一转换为错误码/失败状态并上报；避免在业务逻辑层层捕获导致逻辑分散

### 注释规范

- **避免**在简单、自解释的代码上添加注释（如直观的变量命名、简单的 getter/setter、标准库调用）
- **仅对**以下情况添加精炼注释：
  - 复杂的业务逻辑
  - 关键算法
  - 非直观的设计决策
  - 易出错的部分
- 注释应着重解释 **"为什么这么做"**，而非仅仅描述"做了什么"

### 命名约定

- C API：`Maa<Module><Action>`，如 `MaaTaskerPostTask`
- 枚举：`Maa<Module><Option>Enum`，如 `MaaGlobalOptionEnum`
- 回调：`Maa<Type>Callback`，如 `MaaCustomRecognitionCallback`

### API 设计原则

1. 所有公开 API 使用 C 语言接口
2. 异步操作返回 ID，通过 Wait/Status 接口查询状态
3. 输出参数使用指针，需要用户预分配 Buffer
4. 使用 `MaaBool` 而非 `bool` 作为返回值

### 文档更新规范

- 更新文档时需 **同步更新中英文文档**（`docs/zh_cn/` 和 `docs/en_us/`）
- 如发现文档中有明显错误，应协助修正或提示

### 新增功能检查清单

- [ ] 在 `include/` 中添加 C API 声明
- [ ] 在 `source/` 中实现功能
- [ ] 更新 MaaAgent（如涉及 Context/Tasker/Resource/Controller 接口）
- [ ] 更新 Python/NodeJS 绑定
- [ ] 更新 Pipeline Schema（如涉及）
- [ ] 添加/更新中英文文档
- [ ] 更新 MaaMsg.h 及绑定层消息解析代码（如有新消息）

> **注意**：如没有特别说明，不需要更新测试用例。

## 常见开发场景

### 添加新的 Pipeline 字段

1. 在 `source/MaaFramework/` 添加字段本身的功能性代码
2. 在 `source/MaaFramework/Resource/PipelineParser.cpp` 添加解析逻辑
3. 在 `source/MaaFramework/Resource/PipelineDumper.cpp` 添加序列化逻辑
4. 更新 `source/binding` 中的解析逻辑（注意：binding 中的数据来源于 `PipelineDumper` 序列化的 JSON，而非原始 Pipeline JSON。）
5. 更新 `tools/pipeline.schema.json`
6. 更新中英文文档 `docs/*/3.1-*`

### 修改回调消息（MaaMsg）

当修改或新增回调消息时：

1. 在 `include/MaaFramework/MaaMsg.h` 添加/修改消息定义
2. 在相应的 C++ 代码中发送该消息
3. 更新 Python 绑定中的消息解析代码（`source/binding/Python/maa/` 下的 `event_sink.py`、`tasker.py` 等）
4. 更新中英文回调协议文档 `docs/*/2.3-*`

### 新增 Context/Tasker/Resource/Controller 接口

当为核心对象添加新接口时，需同步更新 MaaAgent 以支持跨进程调用：

1. 在 `source/MaaAgentClient/Client/AgentClient.h` 添加 `handle_<module>_<action>` 方法声明
2. 在 `source/MaaAgentClient/Client/AgentClient.cpp` 实现该方法
3. 在 `source/MaaAgentServer/RemoteInstance/Remote<Module>.h` 添加对应方法声明
4. 在 `source/MaaAgentServer/RemoteInstance/Remote<Module>.cpp` 实现远程调用逻辑

**Agent 架构说明**：

- **AgentClient**：运行在主程序中，将 Custom 请求转发到 Server，并处理 Server 的远程调用
- **AgentServer**：运行在用户进程中，注册自定义识别器/动作，通过 Remote* 类代理访问主程序实例

### 修改 workflows 和 actions

修改 workflows 和 actions 后如果不会自动触发 workflow ，请提醒我 dry run 相关的所有 workflows。

## 参考资源

- [快速开始](docs/zh_cn/1.1-快速开始.md)
- [任务流水线协议](docs/zh_cn/3.1-任务流水线协议.md)
- [集成文档](docs/zh_cn/2.1-集成文档.md)
- [回调协议](docs/zh_cn/2.3-回调协议.md)
- [标准化接口设计](docs/zh_cn/4.2-标准化接口设计.md)
- [Pipeline Schema](tools/pipeline.schema.json)
