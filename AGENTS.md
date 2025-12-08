# MaaFramework 项目指南

本文档旨在帮助开发者（包括 AI）快速了解 MaaFramework 项目结构，以便参与框架本身的开发。

> **注意**：本文档面向希望开发 MaaFramework 本身的贡献者，而非集成使用者。集成使用请参考 [快速开始](docs/zh_cn/1.1-快速开始.md)。

## 项目概述

**MaaFramework** 是一个基于图像识别技术的自动化黑盒测试框架，使用 C++20 编写，提供 C 语言 API 供各语言绑定调用。

- **定位**：既支持通过 JSON 配置任务流水线（Pipeline）的低代码开发，也提供完整的集成接口供深度定制
- **平台支持**：Windows、Linux、macOS、Android
- **开源协议**：LGPL-3.0

## 目录结构

```
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
2. **MaaController**：控制器，负责截图和输入操作（ADB/Win32）
3. **MaaTasker**：任务执行器，绑定 Resource 和 Controller 后执行任务
4. **MaaContext**：任务上下文，在 Custom 回调中提供运行时操作能力

### 执行流程

```
1. 创建 Resource → 加载资源
2. 创建 Controller → 连接设备
3. 创建 Tasker → 绑定 Resource 和 Controller
4. post_task(entry) → 执行任务流水线
```

## Pipeline 协议

### 基本格式

Pipeline 使用 JSON 格式，支持 v1 和 v2 两种写法：

```jsonc
// v1 格式
{
    "NodeName": {
        "recognition": "OCR",       // 识别算法
        "expected": "开始",          // 期望文本
        "action": "Click",          // 执行动作
        "next": ["NextNode1", "NextNode2"]  // 后继节点
    }
}

// v2 格式
{
    "NodeName": {
        "recognition": {
            "type": "TemplateMatch",
            "param": { "template": "image.png" }
        },
        "action": {
            "type": "Click",
            "param": { "target": [100, 100, 50, 50] }
        },
        "next": ["NextNode"]
    }
}
```

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

v5.1+ 支持在 `next` 列表中使用节点属性：

```jsonc
{
    "A": {
        "next": [
            "B",
            "[JumpBack]C",           // 执行完 C 链后返回 A 继续识别
            { "name": "D", "anchor": true }  // 锚点引用
        ]
    }
}
```

## 语言绑定规范

### 标准化接口设计原则

1. **对象化封装**：MaaTasker、MaaResource、MaaController 设计为 OOP 接口
2. **异步任务封装**：返回 Job 类而非原始 ID，提供 `wait()`, `status()`, `get()` 方法
3. **查询 ID 封装**：MaaRecoId、MaaNodeId 等封装为 Detail 结构体
4. **回调封装**：CustomAction、CustomRecognition 包装为虚基类
5. **Buffer 隐藏**：StringBuffer、ImageBuffer 内部使用，不暴露给用户
6. **独立接口**：SetOption 中的枚举拆分为独立方法

### Python 绑定示例（完整集成接口）

```python
from maa.tasker import Tasker
from maa.resource import Resource
from maa.controller import AdbController

# 创建并初始化
resource = Resource()
resource.post_bundle("./resource").wait()

controller = AdbController(adb_path, address)
controller.post_connection().wait()

tasker = Tasker()
tasker.bind(resource, controller)

# 方式一：执行 Pipeline 任务（低代码）
job = tasker.post_task("MyTask")
job.wait()
result = job.get()  # 返回 TaskDetail

# 方式二：直接调用控制器接口（完整集成）
controller.post_click(100, 200).wait()
image = controller.post_screencap().wait().get()
```

### 自定义扩展（Agent 模式）

```python
from maa.agent.agent_server import AgentServer

@AgentServer.custom_recognition("MyReco")
class MyRecognition:
    def analyze(self, ctx, image, node_name, param):
        # 返回识别结果 (box, detail) 或 None
        return (10, 10, 100, 100), "detail_info"

@AgentServer.custom_action("MyAction")
class MyAction:
    def run(self, ctx, node_name, param, box, reco_detail):
        ctx.controller.post_click(100, 100).wait()
        ctx.override_next(["NodeA", "NodeB"])
        return True

AgentServer.start_up(socket_id)
```

## 回调协议

### 消息格式

```cpp
void callback(void* handle, const char* message, const char* details_json, void* trans_arg);
```

### 消息类型

| 消息 | 触发时机 | 关键字段 |
|------|----------|----------|
| `Resource.Loading.*` | 资源加载 | `res_id`, `path`, `hash` |
| `Controller.Action.*` | 控制器动作 | `ctrl_id`, `action`, `param` |
| `Tasker.Task.*` | 任务执行 | `task_id`, `entry` |
| `Node.Recognition.*` | 节点识别 | `task_id`, `reco_id`, `name` |
| `Node.Action.*` | 节点动作 | `task_id`, `action_id`, `name` |
| `Node.NextList.*` | 识别后继列表 | `task_id`, `name`, `list` |

### focus 机制

在 Pipeline 节点中设置 `focus` 字段，可触发额外回调：

```jsonc
{
    "NodeA": {
        "focus": {
            "Node.Recognition.Succeeded": "{name} 识别成功",
            "Node.Action.Starting": "开始执行 {name}"
        }
    }
}
```

## 构建与开发

### 环境要求

- Git、Python 3、CMake 3.24+
- Windows: MSVC 2022
- Linux/macOS: Ninja + g++/clang

### 构建步骤

```bash
# 1. 克隆仓库（含子模块）
git clone --recurse-submodules https://github.com/MaaXYZ/MaaFramework.git

# 2. 下载预编译依赖
python3 tools/maadeps-download.py

# 3. 配置 CMake
cmake --preset "MSVC 2022"   # Windows
cmake --preset "NinjaMulti"  # Linux/macOS

# 4. 构建
cmake --build build --config Release
cmake --install build --prefix install
```

## 开发规范

### 代码风格

- C++ 使用 C++20 标准
- 使用 clang-format 格式化代码
- 头文件使用 `#pragma once`

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

### 添加新的识别算法

1. 在 `source/MaaFramework/Vision/` 添加算法实现
2. 在 `source/MaaFramework/Task/Component/Recognizer.cpp` 注册算法
3. 在 `source/MaaFramework/Resource/PipelineParser.cpp` 添加解析逻辑
4. 在 `source/MaaFramework/Resource/PipelineDumper.cpp` 添加序列化逻辑
5. 更新 Python 绑定中的 `get_node_object` 解析逻辑（如有新的结果格式）
6. 更新 `tools/pipeline.schema.json`
7. 更新中英文文档 `docs/*/3.1-*`

> **说明**：Python 绑定的 `get_node_object` 数据来源于 `PipelineDumper` 序列化的 JSON，而非原始 Pipeline JSON。

### 添加新的动作类型

1. 在 `source/MaaFramework/Task/Component/Actuator.cpp` 实现动作
2. 在 `source/MaaFramework/Resource/PipelineParser.cpp` 添加解析逻辑
3. 在 `source/MaaFramework/Resource/PipelineDumper.cpp` 添加序列化逻辑
4. 更新 Python 绑定中的 `get_node_object` 解析逻辑（如有新的结果格式）
5. 更新 `tools/pipeline.schema.json`
6. 更新中英文文档

> **说明**：Python 绑定的 `get_node_object` 数据来源于 `PipelineDumper` 序列化的 JSON，而非原始 Pipeline JSON。

### 修改回调消息（MaaMsg）

当修改或新增回调消息时：

1. 在 `include/MaaFramework/MaaMsg.h` 添加/修改消息定义
2. 在相应的 C++ 代码中发送该消息
3. 更新 Python 绑定中的消息解析代码（`source/binding/Python/maa/` 下的 `event_sink.py`、`tasker.py` 等）
4. 更新中英文回调协议文档 `docs/*/2.3-*`

### 添加新的语言绑定

参考 `docs/*/4.2-*` 标准化接口设计文档，确保：

1. 对象化封装所有核心类型
2. 异步 ID 封装为 Job 类
3. Buffer 类型内部处理，不暴露给用户
4. 提供与 Python 示例等效的 sample 代码
5. 更新中英文集成文档 `docs/*/2.1-*`

### 新增 Context/Tasker/Resource/Controller 接口

当为核心对象添加新接口时，需同步更新 MaaAgent 以支持跨进程调用：

1. 在 `source/MaaAgentClient/Client/AgentClient.h` 添加 `handle_<module>_<action>` 方法声明
2. 在 `source/MaaAgentClient/Client/AgentClient.cpp` 实现该方法
3. 在 `source/MaaAgentServer/RemoteInstance/Remote<Module>.h` 添加对应方法声明
4. 在 `source/MaaAgentServer/RemoteInstance/Remote<Module>.cpp` 实现远程调用逻辑

**Agent 架构说明**：

- **AgentClient**：运行在主程序中，将 Custom 请求转发到 Server，并处理 Server 的远程调用
- **AgentServer**：运行在用户进程中，注册自定义识别器/动作，通过 Remote* 类代理访问主程序实例

## 调试技巧

### 日志配置

```python
Tasker.set_log_dir("./logs")
Tasker.set_stdout_level(LoggingLevelEnum.All)
Tasker.set_debug_mode(True)
Tasker.set_save_draw(True)
```

### maa_option.json

```json
{
    "logging": true,
    "save_draw": true,
    "stdout_level": 7,
    "save_on_error": true
}
```

## 参考资源

- [快速开始](docs/zh_cn/1.1-快速开始.md)
- [任务流水线协议](docs/zh_cn/3.1-任务流水线协议.md)
- [集成文档](docs/zh_cn/2.1-集成文档.md)
- [回调协议](docs/zh_cn/2.3-回调协议.md)
- [标准化接口设计](docs/zh_cn/4.2-标准化接口设计.md)
- [Pipeline Schema](tools/pipeline.schema.json)
