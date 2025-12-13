# MaaFramework Rust 绑定

MaaFramework 的 Rust 语言绑定，遵循标准化接口设计规范。

> ⚠️ **注意**：此绑定完全由 AI 生成，尚未经过充分测试。请开发者在使用时自行验证其正确性和稳定性。

## 特性

- 🦀 纯 Rust 实现，类型安全
- 📦 面向对象的 API 设计
- ⚡ 异步任务支持 (Job 模式)
- 🔄 完整的回调/通知系统
- 🛠️ 自定义识别器/动作支持

## 安装

```toml
[dependencies]
maa-framework = { path = "source/binding/Rust" }
serde_json = "1.0"
```

## 快速开始

```rust
use maa_framework::*;
use serde_json::json;

fn main() -> Result<()> {
    // 1. 加载库
    load_library("MaaFramework.dll")?;
    load_toolkit("MaaToolkit.dll")?;

    // 2. 设置日志
    set_log_dir("./log");

    // 3. 查找设备
    let devices = Toolkit::find_adb_devices();
    let device = devices.first().expect("未找到设备");

    // 4. 创建控制器并连接
    let controller = AdbController::new(
        &device.adb_path,
        &device.address,
        adb_screencap_method::DEFAULT,
        adb_input_method::DEFAULT,
        &device.config,
        "MaaAgentBinary",
    )?;
    controller.post_connection().wait();

    // 5. 加载资源
    let resource = Resource::new()?;
    resource.post_bundle("./resource").wait();

    // 6. 创建任务器
    let mut tasker = Tasker::new()?;
    tasker.bind(resource, controller);

    // 7. 执行任务
    let job = tasker.post_task("StartUp", &json!({}))?;
    job.wait();

    if job.succeeded() {
        println!("✅ 任务成功");
    }

    Ok(())
}
```

## 模块说明

| 模块 | 说明 |
|------|------|
| `define` | 类型定义、枚举常量 |
| `buffer` | 缓冲区类型 (StringBuffer, ImageBuffer 等) |
| `job` | 异步任务封装 (TaskJob, CtrlJob, ResJob) |
| `resource` | 资源管理 |
| `controller` | 控制器 (ADB, Win32, Debug) |
| `tasker` | 任务执行器 |
| `context` | 任务上下文 |
| `toolkit` | 工具函数 (设备发现等) |
| `custom` | 自定义识别/动作接口 |
| `notification` | 回调通知 |

## Job 模式

所有异步操作返回 Job 对象，提供统一的状态查询接口：

```rust
let job = controller.post_connection();

// 等待完成
job.wait();

// 查询状态
if job.succeeded() { /* ... */ }
if job.failed() { /* ... */ }
if job.pending() { /* ... */ }
if job.running() { /* ... */ }
```

## 通知消息

支持所有标准通知消息类型：

```rust
use maa_framework::notification::*;

// 消息类型
msg::RESOURCE_LOADING_STARTING
msg::CONTROLLER_ACTION_SUCCEEDED
msg::TASKER_TASK_FAILED
msg::NODE_RECOGNITION_SUCCEEDED
// ...

// 解析通知
let noti_type = parse_notification_type(message);
let detail = parse_tasker_task(details_json);
```

## 截图方式

### ADB 控制器

```rust
use maa_framework::adb_screencap_method::*;

// 默认方式 (推荐)
DEFAULT

// 单独方式
ENCODE_TO_FILE_AND_PULL
ENCODE
RAW_WITH_GZIP
RAW_BY_NETCAT
MINICAP_DIRECT
MINICAP_STREAM
EMULATOR_EXTRAS
```

### Win32 控制器

```rust
use maa_framework::win32_screencap_method::*;

GDI
FRAME_POOL
DXGI_DESKTOP_DUP
DXGI_DESKTOP_DUP_WINDOW
PRINT_WINDOW
SCREEN_DC
```

## 推理设备设置

```rust
let resource = Resource::new()?;

// CPU
resource.use_cpu();

// DirectML (Windows GPU)
resource.use_directml(0); // GPU ID

// CoreML (macOS)
resource.use_coreml(0);

// 自动选择
resource.use_auto_ep();
```

## 与 Python/C# 绑定对比

| 功能 | Python | C# | Rust |
|------|--------|------|------|
| 异步任务 | Job 类 | Job 类 | Job 结构体 |
| 类型安全 | 运行时 | 编译时 | 编译时 |
| 内存管理 | GC | GC | RAII |
| 回调处理 | EventSink | EventSink | Trait |
| 错误处理 | 异常 | 异常 | Result |

## 许可证

MIT License

