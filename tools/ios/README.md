# iOS device management API / iOS 设备管理 API

There is no separate device-manager executable. Applications call MaaToolkit; it loads
MaaIOSControlUnit through LibraryHolder. Protocol code remains private to the control unit.

不再提供独立设备管理程序。应用调用 MaaToolkit，由 LibraryHolder 加载 MaaIOSControlUnit。
协议、配对和 DDI 操作均位于控制单元内部。

## Credentials / 配对凭据

`MaaToolkitIOSDevicePair(options, event, cancelled, userdata, result, credential)` takes
two distinct caller-created `MaaStringBuffer` outputs. `result` contains status JSON;
`credential` contains an opaque versioned credential only after successful verification.
A failed operation clears the credential output. Destroy both buffers through MaaFramework.
The caller chooses whether and where to persist the credential. Do not log its contents.

配对使用两个不同的调用方缓冲区：`result` 返回状态，`credential` 返回验证后的不透明、
带版本凭据。保存位置与保存方式由调用方决定；框架不创建或扫描默认配对目录，
不自动迁移旧文件。凭据包含密钥，普通结果、设备列表和进度事件均不包含其内容。

Pass exported credential strings as `pairing_records: [credential, ...]` in Discover,
CheckConnection, Prepare options or the controller's `config` JSON. Each operation copies
its inputs; controllers own their configuration, so the original buffers can be destroyed.
Without credentials, mDNS discovery still lists advertisements but cannot mark them paired.
Credentials must be complete, at most 16 KiB each, and have unique device UDIDs.

发现、连接检查、DDI 准备和控制器配置均显式传入 `pairing_records` 字符串数组。
控制器持有自己的副本；创建后可释放原始缓冲区。无凭据也能发现广播，但不会标记已配对。
PiCli 将凭据直接保存在 `config/maa_pi_config.json` 的 `ios.pairing_records` 数组中，
用于设备发现和控制器连接，无需配置单独的凭据文件路径。

PiCli stores credentials directly in `ios.pairing_records` in `config/maa_pi_config.json`
and uses them for discovery and controller connections; no separate credential file path is needed.

PiCli 配置 iOS 控制器时，可选择使用已有配对、USB 配对或 Wi-Fi 配对。
USB 配对需在设备上确认信任；Wi-Fi 配对需在开发者模式页面选择电脑，并输入终端显示的配对码。
成功后自动保存凭据；失败不修改已保存的凭据。

When configuring an iOS controller in PiCli, choose existing pairing, USB pairing or Wi-Fi pairing.
Accept trust prompts for USB; for Wi-Fi, select the computer on the Developer Mode page
and enter the code printed in the terminal. Successful pairing saves credentials automatically;
failure leaves existing credentials unchanged.


```python
from maa.toolkit import Toolkit
from maa.controller import IOSController

# Print only the PIN/progress event, never the returned credential.
def on_event(event):
    print(event.get("pin", event.get("message", "")))
    return True

status, credential = Toolkit.ios_pair({"wifi": "auto"}, on_event=on_event)
if not status["success"]:
    raise RuntimeError(status["error"])

# The application may persist `credential` securely, or keep it only in memory.
options = {"wifi": "auto", "pairing_records": [credential]}
status = Toolkit.ios_prepare(options)  # Temporary downloads by default.
if not status["success"]:
    raise RuntimeError(status["error"])
devices = Toolkit.find_ios_devices(pairing_records=[credential])
controller = IOSController(wifi_address="auto", pairing_records=[credential])
```

USB pairing omits `wifi`; Wi-Fi first pairing uses `wifi="auto"` and requires an event
callback that displays the PIN. Existing credentials may be supplied for USB verification;
`repair=true` explicitly permits repairing a device-rejected credential. Pair timeout is
1..300000 ms (default 120000). Discovery uses timeout_ms 0..60000 (default 3000).
CheckConnection validates authentication/tunnel/RSD and closes its own session on return.
It does not provide a shared tunnel or persistent server.

USB 配对不填写 `wifi`；无线首次配对填写 `"auto"` 并显示 PIN。USB 可传入已有凭据进行验证；
仅显式 `repair=true` 才允许修复被设备拒绝的凭据。连接检查完成后释放会话，控制器自行建连。

## Optional DDI cache / 可选 DDI 缓存

Prepare downloads and installs a DDI using native device services. Without `cache_directory`,
assets live in a unique temporary directory and are removed after completion, failure or
cancellation. Set `cache_directory` to retain and revalidate files across calls. An applicable
mounted CoreDevice image with the required screenshot feature can be reused without downloading.
When compatibility cannot be confirmed, normal image validation/preparation is still required.
`replace` defaults to false. Download/install timeout defaults are 300/180 seconds.

不指定 `cache_directory` 时使用临时目录，完成、失败或取消后清理；只有显式指定目录才启用
持久缓存。已确认适用的设备挂载镜像可直接复用；无法确认时继续校验准备流程。
`replace` 默认关闭，不能因取消就假定设备安装已回滚。

## Build / 构建

Configure the MaaFramework repository root with `WITH_IOS_CONTROLLER=ON`. Supply normal
MaaDeps plus target-compatible OpenSSL, FFmpeg and libcurl. `ci/` holds dependency recipes
and runtime/source packaging scripts, not a separate protocol or manager project.
Python exposes management calls. NodeJS exposes controller/discovery configuration; interactive
pairing/preparation is currently available through C/Python APIs.

从仓库根目录启用 `WITH_IOS_CONTROLLER=ON` 构建。`ci/` 仅保留依赖和框架打包配方。
Python 提供设备管理方法；NodeJS 提供控制器和发现配置，交互式配对/准备目前通过 C/Python API 调用。

## PiCli 自动准备 DDI / Automatic DDI preparation in PiCli

PiCli 运行 iOS 任务前会调用 `MaaToolkitIOSDevicePrepare`，成功后才创建并连接控制器。
设备上可确认适用的已挂载镜像会被复用；否则按需下载、安装并验证挂载。
准备失败时显示错误并停止运行。默认使用临时下载文件，不启用持久缓存，不主动替换已有镜像。
仅完成配对不会触发 DDI 准备。

Before running iOS tasks, PiCli calls `MaaToolkitIOSDevicePrepare` and proceeds to controller
creation and connection only on success. It reuses a confirmed suitable mounted image or
performs the required download, installation and mount verification. Preparation errors stop
the run. Downloads use temporary storage by default, with no persistent cache or explicit
replacement of an existing image. Pairing alone does not trigger preparation.
