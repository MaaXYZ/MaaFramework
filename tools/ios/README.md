# iOS device management API / iOS 设备管理 API

MaaToolkit implements discovery, pairing, connection checks and DDI preparation.
MaaToolkit and MaaIOSControlUnit share a private static protocol target.

发现、配对、连接检查和 DDI 准备由 MaaToolkit 实现。
MaaToolkit 与 MaaIOSControlUnit 共用内部协议静态目标。

## Credentials / 配对凭据

`MaaToolkitIOSDevicePair(udid, method, existing_pairing, options, event, cancelled, userdata, result, &pairing)` receives
status JSON in a caller-created `MaaStringBuffer` and returns an owned `MaaIOSWiFiPairing*`.
On failure `pairing` is NULL. Destroy successful outputs with `MaaToolkitIOSDeviceDestroyPairing`.
Its identity-string pointers remain valid until destruction; controllers copy all data during creation.
The structure contains the device UDID, host identifier and peer identifier as
NUL-terminated UTF-8 text, followed by three 32-byte signing keys and a 16-byte peer IRK array
with `has_peer_alt_irk` to indicate presence. Keys are fixed-size inline arrays.
UDID and host identifier accept 1–1024 UTF-8 bytes; peer identifier accepts 1–4096.
Identity strings cannot contain NUL or line breaks.

配对的 `result` 返回状态 JSON，`pairing` 返回已验证的持有型 `MaaIOSWiFiPairing*`，失败时为空。
使用 `MaaToolkitIOSDeviceDestroyPairing` 释放。字段指针在对象释放前有效，控制器创建时复制所需数据。
结构体依次包含设备 UDID、主机标识、设备身份文本（均为以 NUL 结尾的 UTF-8），
再放三份 32 字节密钥数组及 16 字节 IRK 数组；`has_peer_alt_irk` 标记 IRK 是否提供。
UDID 和主机标识的长度为 1–1024 个 UTF-8 字节，设备身份为 1–4096 字节，均不能含 NUL 或换行。

For optional persistence, `MaaToolkitIOSDeviceFormatPairingRecord(pairing, buffer)` writes
the versioned `MaaIOS1` string. `peer_identifier` is stored as plain text;
the four key fields use hexadecimal encoding. `MaaToolkitIOSDeviceParsePairingRecord(text)`
returns a new owned structure. These operations do not contact the device. The controller
accepts the structure directly, without JSON or hex decoding. Controller creation requires a nonempty UDID
that matches the wireless pairing.

需要持久化时用 `MaaToolkitIOSDeviceFormatPairingRecord` 导出，恢复时用
`MaaToolkitIOSDeviceParsePairingRecord`，格式头为 `MaaIOS1`。
`peer_identifier` 保存文本原文，四个密钥字段使用十六进制。
转换接口不访问设备；控制器直接接收结构体，不解析 JSON 或十六进制凭据字符串。
创建控制器必须填写非空 UDID，且与提供的无线凭据一致。

The single-line format uses `|` between fields, without a trailing separator:

```text
MaaIOS1|udid=...|hid=...|pid=...|hsk=...|hpk=...|ppk=...|irk=...
```

| Field / 字段 | Structure member / 结构体成员 |
| --- | --- |
| `udid` | `udid` |
| `hid` | `host_identifier` |
| `pid` | `peer_identifier` |
| `hsk` | `host_private_key` |
| `hpk` | `host_public_key` |
| `ppk` | `peer_public_key` |
| `irk` | `peer_alt_irk` (optional / 可选) |

The format header is `MaaIOS1`. Text values encode `%` as `%25` and `|` as `%7C`;
key bytes use hexadecimal. Line breaks, duplicate fields, unknown fields and malformed
escapes are rejected.

单行格式头为 `MaaIOS1`，字段间使用 `|`，结尾不添加分隔符。
文本值中的 `%`、`|` 分别编码为 `%25`、`%7C`；密钥使用十六进制。
解析拒绝换行、重复字段、未知字段和无效转义。

Device operations accept structured credentials directly:

| Operation / 操作 | Credentials / 凭据 |
| --- | --- |
| Discover / FindWithOptions | `const MaaIOSWiFiPairing* const*` + count; up to 256 unique UDIDs / 最多 256 条、UDID 不重复 |
| Pair | One optional existing pairing: USB verification/reuse, Wi-Fi duplicate protection / 可选的一条已有凭据：USB 验证复用、无线防止重复配对 |
| CheckConnection / Prepare | One target `const MaaIOSWiFiPairing*` / 目标设备的单条凭据 |

All operations take an explicit `MaaIOSConnectionMethod`. Connection checks and DDI preparation
also require a nonempty `udid` and take `wifi_address`, just like the controller. Pairing uses a transport and UDID;
Wi-Fi pairing advertises the host instead of connecting to a device address. Discovery works
without credentials, but cannot mark unmatched advertisements as paired. JSON options contain
only operation settings (timeouts, cache and repair).
Inputs are copied before device I/O and borrowed only until the synchronous call returns.

所有设备操作显式接收 `MaaIOSConnectionMethod`。连接检查和 DDI 准备与控制器一样，
必须填写非空 `udid`；`wifi_address` 和无线凭据按连接方式提供。配对传入连接方式和 UDID；无线配对广播主机，
不指定设备地址。无凭据也能发现广播，但不会标记为已配对。
JSON 选项包含超时、缓存、修复等操作设置。
输入在设备 I/O 前复制，调用返回后可释放。

PiCli saves credential strings in `ios.pairing_records` in `config/maa_pi_config.json`.
It parses them when loading credentials, then passes structures to discovery, pairing,
DDI preparation and controller creation. Formatting is used only when saving credentials.

PiCli 在 `config/maa_pi_config.json` 的 `ios.pairing_records` 中保存凭据字符串；
加载时解析，发现、配对、DDI 准备和控制器创建都直接传结构体，仅保存时序列化。

Connection modes / 连接方式：

- Auto: physical USB first for the specified UDID, then RemotePairing / 对指定 UDID 优先物理 USB，没有候选时使用无线。
- USB: physical USB only; wireless parameters are ignored / 强制 USB，忽略无线参数，使用系统信任。
- WiFi: RemotePairing only, requires structured credentials; an empty address enables mDNS discovery / 强制无线，地址留空自动发现。

```c
MaaStringBuffer* status = MaaStringBufferCreate();
MaaIOSWiFiPairing* pairing = NULL;
/* event_callback displays the PIN; see the callback contract below. */
if (MaaToolkitIOSDevicePair(
        NULL, MaaIOSConnectionMethod_WiFi, NULL, NULL,
        event_callback, NULL, NULL, status, &pairing)) {
    MaaController* controller = MaaIOSControllerCreate(
        pairing->udid, MaaIOScreencapMethod_Default, MaaIOSConnectionMethod_WiFi, NULL, pairing);
    MaaToolkitIOSDeviceDestroyPairing(pairing);
    /* controller now owns a copy; connect with MaaControllerPostConnection. */
    /* Destroy the controller when finished. */
    MaaControllerDestroy(controller);
}
MaaStringBufferDestroy(status);
```

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
from maa.define import MaaIOSConnectionMethodEnum

# Print only the PIN/progress event, never the returned credential.
def on_event(event):
    print(event.get("pin", event.get("message", "")))
    return True

status, pairing = Toolkit.ios_pair(
    connection_method=MaaIOSConnectionMethodEnum.WiFi, on_event=on_event,
)
if not status["success"]:
    raise RuntimeError(status["error"])

# Optional persistence; pass the structure directly for device operations.
credential = Toolkit.format_ios_pairing_record(pairing)
status = Toolkit.ios_prepare(
    udid=pairing.udid, connection_method=MaaIOSConnectionMethodEnum.WiFi, wifi_pairing=pairing,
)  # Temporary downloads by default.
if not status["success"]:
    raise RuntimeError(status["error"])
devices = Toolkit.find_ios_devices(wifi_pairings=[pairing])
controller = IOSController(
    udid=pairing.udid, connection_method=MaaIOSConnectionMethodEnum.WiFi, wifi_pairing=pairing,
)
# After loading a saved string: pairing = Toolkit.parse_ios_pairing_record(credential)
```

Pair with `connection_method=USB` or `WiFi`; Wi-Fi first pairing requires an event
callback that displays the PIN. Auto prefers physical USB, then advertises for Wi-Fi pairing. Existing credentials may be supplied for USB verification;
`repair=true` explicitly permits repairing a device-rejected credential. Pair timeout is
1..300000 ms (default 120000). Discovery uses timeout_ms 0..60000 (default 3000).
CheckConnection validates authentication/tunnel/RSD and closes its own session on return.
It does not provide a shared tunnel or persistent server.

配对显式选择 `USB` 或 `WiFi`；`Auto` 优先物理 USB，否则广播无线配对。无线配对需要显示 PIN。USB 可传入已有凭据进行验证；
仅显式 `repair=true` 才允许修复被设备拒绝的凭据。连接检查完成后释放会话，控制器自行建连。

## Optional DDI cache / 可选 DDI 缓存

Prepare downloads and installs a DDI using native device services. Without `cache_directory`,
assets live in a unique temporary directory and are removed after completion, failure or
cancellation. Set `cache_directory` to retain and revalidate files across calls. An applicable
mounted CoreDevice image with the required screenshot feature can be reused without downloading.
Classic images are reused without downloading when their mounted signature matches the pinned catalog for the device OS.
When compatibility cannot be confirmed, image validation and preparation are required.
`replace` defaults to false. A different or unverifiable mounted Classic signature fails;
set `replace: true` to explicitly replace it. Download/install timeout defaults are 300/180 seconds.

不指定 `cache_directory` 时使用临时目录，完成、失败或取消后清理；只有显式指定目录才启用
持久缓存。已确认适用的设备挂载镜像可直接复用；Classic 镜像的挂载签名与设备系统对应的固定目录摘要匹配时，
无需下载或本地缓存。无法确认时继续校验准备流程。
`replace` 默认关闭。Classic 挂载签名不匹配或无法验证时返回错误；显式设置 `replace: true` 才会替换。
不能因取消就假定设备安装已回滚。

## Build / 构建

Configure the MaaFramework repository root with `WITH_IOS_CONTROLLER=ON`. Supply normal
MaaDeps plus target-compatible OpenSSL, FFmpeg and libcurl. The framework `build.yml` uses
`ci/` recipes to build and package these dependencies for each target. `test.yml` runs the offline
iOS tests and Python management tests. Artifacts include runtime licenses, dependency sources
and build recipes.
Python exposes management calls. NodeJS exposes controller/discovery configuration; interactive
pairing/preparation is currently available through C/Python APIs.

从仓库根目录启用 `WITH_IOS_CONTROLLER=ON` 构建。框架 `build.yml` 使用 `ci/` 配方构建并打包各目标平台的依赖；
`test.yml` 运行 iOS 离线测试和 Python 管理接口测试。产物包含运行库许可证、依赖源码和构建配方。
Python 提供设备管理方法；NodeJS 提供控制器和发现配置，交互式配对/准备通过 C/Python API 调用。

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
缓存只避免重复下载；个性化签票可能需要访问网络。
准备期间 Ctrl+C 请求取消，等待清理后以 130 退出；底层操作按自身的取消检查点响应。

PiCli uses the platform cache directories listed above for persistent DDI downloads.
In the `ios` section of `config/maa_pi_config.json`, set `prepare_ddi` to `false` to skip preparation
when the device is already prepared (including offline use). Empty `ddi_cache_directory` selects
the platform default; an override must be absolute. `ddi_download_timeout_seconds` accepts 1–3600,
defaulting to 300. Cached assets avoid repeat downloads, but personalization may require network access.
During preparation, Ctrl+C requests cancellation; PiCli exits with code 130 after cleanup.
Underlying operations respond at their cancellation checkpoints. These defaults belong to PiCli;
Toolkit callers choose their own cache location or temporary storage.

Android arm64 主机的视频解码依赖构建见 [构建指南](../../docs/zh_cn/4.1-构建指南.md#android-主机的-ios-视频流)。
For Android arm64 video-decoding dependencies, see the [build guide](../../docs/en_us/4.1-BuildGuide.md#ios-video-streaming-on-an-android-host).
