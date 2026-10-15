# MaaIOSManager

MaaFramework's iOS device management executable. It calls the public MaaToolkit C API;
Toolkit loads MaaIOSControlUnit through the framework's LibraryHolder, just as other
controller implementations are loaded. All device protocols, pairing, connection checks
and DDI operations live privately inside MaaIOSControlUnit. There is no separately
built or distributed protocol library, and Manager does not compile protocol code.
It does not require scrctl binaries, libraries or a sibling checkout.

## Commands

```sh
MaaIOSManager devices --json
MaaIOSManager pair -s DEVICE_UDID
MaaIOSManager pair --wifi
MaaIOSManager connect -s DEVICE_UDID --wifi auto --json
MaaIOSManager connect -s DEVICE_UDID --wifi 192.168.1.42 --port 49152
MaaIOSManager prepare -s DEVICE_UDID
```

- `devices` takes a USB/mDNS snapshot. One device may have multiple IPv4, IPv6 and
  interface candidates; JSON retains each candidate. Unknown wireless devices have an
  empty UDID until authenticated pairing matches their identity.
- `pair` defaults to trusted USB pairing. Unlock the phone and confirm any trust or remote
  pairing request. `pair --wifi` advertises a temporary computer; select it in the phone's
  developer pairing page and enter the one-use code printed by the tool. `--repair`
  allows USB repair of an unusable record and is not valid with `--wifi`.
- `connect` checks authentication, tunnel setup and RSD discovery, **then closes the
  session and exits**. `session_open` is always false in the result. It is not an ADB
  server and does not make a tunnel reusable by another process. Controllers independently
  create their sessions from the same pairing records. No `disconnect` command is needed.
- `prepare` selects and downloads a verified DDI for the connected OS/model, then installs
  it using native device services. Images are downloaded when needed, not bundled into the
  executable. Without `--replace`, an incompatible mounted image is not removed. A failed
  or cancelled installation may already have modified the phone; inspect state before
  retrying with replacement enabled.

Discovery timeout defaults to 3 seconds, pairing to 120 seconds. Connection preparation
has a 30-second budget. DDI download/installation default to 300/180 seconds and have
separate `--download-timeout` / `--install-timeout` options. Ctrl+C requests cancellation
and exits with status 130. Invalid arguments exit with status 2; other failures use 1.
Device output does not include pairing secrets.

Pairing records and DDI caches use MaaFramework's platform data/cache directory under
`MaaFramework/iOS`, independently of scrctl records. `prepare --cache-dir PATH` overrides
the DDI cache directory.

## Build and runtime

Configure the repository root after supplying the normal MaaDeps and the additional
iOS OpenSSL, FFmpeg and libcurl dependencies. `WITH_IOS_CONTROLLER` defaults to `OFF`.

```sh
cmake -S . -B build -DWITH_IOS_CONTROLLER=ON -DBUILD_IOS_MANAGER_TESTING=ON
cmake --build build --target MaaIOSManager --parallel
ctest --test-dir build -R MaaIOSManager --output-on-failure
cmake --install build --prefix install
```

Manager is built with MaaFramework, rather than configured as a separate lightweight
project. Deploy it with MaaToolkit, MaaFramework, MaaUtils, MaaIOSControlUnit and their
runtime dependencies; copying only the executable is insufficient. The protocol sources
are private sources of MaaIOSControlUnit and its OpenSSL/FFmpeg/libcurl dependencies do
not become a new public C++ API.

## Cross-platform CI

The `iOS manager` workflow explicitly enables iOS support and builds the complete
framework C API, toolkit, control-unit session and Manager on native macOS ARM64,
Linux x64 and Windows MSVC x64. Protocol tests use an internal test-only support archive instead of
linking hidden C++ symbols from a shared library; a separate test calls the real exported
management C API. No neighboring scrctl checkout is used.

The artifact contains the framework and Manager runtime, source and dependency notices.
Windows uses the pinned vcpkg manifest in `ci/vcpkg.json`, with DLL ownership/hashes,
source archives and recipes/patches. Microsoft VC++ x64 runtime is required. Unix
artifacts include MaaDeps libraries and use system OpenSSL/FFmpeg/libcurl packages.
These are offline build/API checks, not phone or virtual-machine compatibility tests.

## 中文说明

`MaaIOSManager` 是 MaaFramework 自有的 iOS 设备管理程序，与控制器共用配对记录，
通过公开 MaaToolkit C API 调用设备管理能力；Toolkit 按框架原有 LibraryHolder 机制
加载 MaaIOSControlUnit。协议、配对、连接检查和 DDI 操作均封装在控制单元内部，
不再构建或分发独立协议库，Manager 本身也不编译协议源码。
不运行 scrctl，也不依赖旁边的 scrctl 源码仓库。

- `devices` 列举 USB 和 Wi-Fi 设备，保留同一设备的全部地址与接口候选；`--json` 输出结构化快照。
- `pair` 默认通过 USB 配对，需要手机解锁并确认信任或远程配对提示。`pair --wifi`
  广播临时电脑，需在手机开发者配对页面选择它，并输入终端显示的一次性配对码。
- `connect` 验证认证、隧道与 RSD 服务目录，然后关闭连接并退出。
  本期不提供常驻服务或跨进程共享隧道，所以无需 `disconnect`；控制器会自行建立连接。
- `prepare` 按设备系统版本与机型下载并校验 DDI，再通过原生协议安装。
  默认不替换不兼容的已挂载镜像；`--replace` 才允许替换。安装中断可能已在设备端生效。

配对记录和镜像缓存放在平台目录的 `MaaFramework/iOS` 下，与 scrctl 独立。
Ctrl+C 可以取消；取消退出码为 130，参数错误为 2，其他错误为 1。
构建从 MaaFramework 仓库根目录进行，命令见上文；`WITH_IOS_CONTROLLER` 默认关闭，
启用前需提供正常 MaaDeps 与额外 iOS 依赖。分发 Manager 时需同时携带 Toolkit、
Framework、MaaUtils、MaaIOSControlUnit 及相应运行库，单独复制可执行文件不足以运行。

CI 只保留三平台完整框架验证，覆盖框架 C API、Toolkit、控制单元会话、Manager 及内部协议测试。
协议测试使用内部专用测试归档，不要求共享库导出 C++ 协议符号；管理 C API 有独立真实入口测试。
Windows 工件包含 DLL、许可证、来源与哈希、源代码归档及构建配方/补丁，
仍需 Microsoft VC++ x64 运行库；Unix 工件包含 MaaDeps 运行库并使用系统协议依赖。
CI 离线测试不等于手机或虚拟机的实机兼容验证。

## API integration

C applications use MaaToolkit's public iOS management C APIs. Python exposes
`Toolkit.ios_discover`, `ios_pair`, `ios_check_connection` and `ios_prepare`, accepting
options dictionaries and returning result dictionaries. Optional `on_event` callbacks
receive event dictionaries (`True` continues, `False` cancels); cancellation callbacks
may run on a native worker and must be thread safe. The binding cancels native work and
rethrows callback exceptions after that work returns. NodeJS currently retains device
discovery/controller bindings; interactive management is available through Manager or
C API integration rather than new NodeJS management methods.

C 程序通过 MaaToolkit 公开 iOS 管理接口接入。Python 提供 `Toolkit.ios_discover`、
`ios_pair`、`ios_check_connection`、`ios_prepare`，接收选项字典并返回结果字典。
可选事件回调接收字典，返回 `True` 继续、`False` 取消；取消回调可能由原生工作线程执行，
需保证线程安全。回调异常会先取消并等原生调用结束，再由 Python 重新抛出。
NodeJS 本期保留设备发现及控制器绑定；交互管理请使用 Manager 或接入 C API，
没有新增 NodeJS 管理方法。
