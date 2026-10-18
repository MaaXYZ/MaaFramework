# iOS device management API / iOS 设备管理 API

MaaToolkit implements discovery, pairing, connection checks and DDI preparation.
MaaToolkit and MaaIOSControlUnit share a private static protocol target.

发现、配对、连接检查和 DDI 准备由 MaaToolkit 实现。
MaaToolkit 与 MaaIOSControlUnit 共用内部协议静态目标。

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
and runtime/source packaging scripts.
Python exposes management calls. NodeJS exposes controller/discovery configuration; interactive
pairing/preparation is currently available through C/Python APIs.

从仓库根目录启用 `WITH_IOS_CONTROLLER=ON` 构建。`ci/` 仅保留依赖和框架打包配方。
Python 提供设备管理方法；NodeJS 提供控制器和发现配置，交互式配对/准备目前通过 C/Python API 调用。

## PiCli 自动准备 DDI / Automatic DDI preparation in PiCli

PiCli 运行 iOS 任务前会调用 `MaaToolkitIOSDevicePrepare`，成功后才创建并连接控制器。
设备上可确认适用的已挂载镜像会被复用；否则按需下载、安装并验证挂载。
准备失败时显示错误并停止运行，不主动替换已有镜像。
仅完成配对不会触发 DDI 准备。

Before running iOS tasks, PiCli calls `MaaToolkitIOSDevicePrepare` and proceeds to controller
creation and connection only on success. It reuses a confirmed suitable mounted image or
performs the required download, installation and mount verification. Preparation errors stop
the run. It does not explicitly replace an existing image. Pairing alone does not trigger preparation.

PiCli 默认使用当前用户的平台持久缓存目录：

| 平台 / Platform | 默认目录 / Default directory |
| --- | --- |
| macOS | `~/Library/Caches/MaaPiCli/ddi/` |
| Linux | `$XDG_CACHE_HOME/MaaPiCli/ddi/`，未设置或非绝对路径时用 `~/.cache/MaaPiCli/ddi/` |
| Windows | `%LOCALAPPDATA%/MaaPiCli/cache/ddi/` |

在 `config/maa_pi_config.json` 的 `ios` 中可配置：

```json
{
    "prepare_ddi": true,
    "ddi_cache_directory": "",
    "ddi_download_timeout_seconds": 300
}
```

`prepare_ddi: false` 跳过准备，由用户保证设备已就绪；可用于已准备好的离线设备。
`ddi_cache_directory` 留空使用上表默认值，也可指定绝对路径。下载超时范围为 1–3600 秒。
缓存只避免重复下载；需要个性化签票时仍可能访问网络。
准备期间 Ctrl+C 请求取消，等待清理后以 130 退出；底层操作按自身的取消检查点响应。

PiCli uses the platform cache directories listed above for persistent DDI downloads.
In the `ios` section of `config/maa_pi_config.json`, set `prepare_ddi` to `false` to skip preparation
when the device is already prepared (including offline use). Empty `ddi_cache_directory` selects
the platform default; an override must be absolute. `ddi_download_timeout_seconds` accepts 1–3600,
defaulting to 300. Cached assets avoid repeat downloads, but personalization may still require network access.
During preparation, Ctrl+C requests cancellation; PiCli exits with code 130 after cleanup.
Underlying operations respond at their cancellation checkpoints. These defaults belong to PiCli;
Toolkit callers still choose their own cache location or temporary storage.

Android arm64 主机的视频解码依赖构建见 [构建指南](../../docs/zh_cn/4.1-构建指南.md#android-主机的-ios-视频流)。
For Android arm64 video-decoding dependencies, see the [build guide](../../docs/en_us/4.1-BuildGuide.md#ios-video-streaming-on-an-android-host).
