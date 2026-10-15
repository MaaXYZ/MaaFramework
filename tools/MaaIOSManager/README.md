# MaaIOSManager

MaaFramework's independent iOS device management executable. It uses the repository's
`MaaIOSProtocol` implementation and shares pairing records with iOS controllers. It does
not run `scrctl`, link an installed `libscrctl`, or require a sibling scrctl checkout.

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

## Standalone build

```sh
cmake -S tools/MaaIOSManager -B build-ios-manager -DBUILD_IOS_MANAGER_TESTING=ON
cmake --build build-ios-manager --parallel
ctest --test-dir build-ios-manager --output-on-failure
```

The standalone configuration needs the protocol's OpenSSL, libcurl, FFmpeg and pinned
header/network dependencies. It does not configure the complete MaaFramework or OpenCV.
Protocol CMake supports `MAA_IOS_FETCH_DEPENDENCIES=OFF` with explicit FetchContent source
directories for offline builds. `cmake --install build-ios-manager --prefix OUTPUT` installs
the executable to `OUTPUT/bin`.

## Cross-platform CI

The independent `iOS manager` workflow builds on macOS, Linux and Windows x64
(MSVC), running command and protocol tests without a neighboring scrctl checkout.
Windows uses the pinned vcpkg manifest in `ci/vcpkg.json`; its artifact includes
runtime DLLs, package ownership/hashes, dependency notices, source archives and
recipes/patches. Microsoft VC++ x64 runtime is still required. Unix artifacts use
system package libraries rather than bundling them. A separate native framework job explicitly enables `WITH_IOS_CONTROLLER` and compiles
the framework API, toolkit and controller session against MaaDeps. The framework option
defaults to `OFF` until its additional dependencies are provided. These jobs run offline
checks; they do not establish phone or virtual-machine compatibility.

## 中文说明

`MaaIOSManager` 是 MaaFramework 自有的 iOS 设备管理程序，与控制器共用配对记录，
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
独立构建命令见上文，无需构建 MaaFramework 全部模块或配置 OpenCV。

独立 CI 覆盖 macOS、Linux 与 Windows x64 MSVC，运行 CLI 和协议离线测试，
不需要旁边的 scrctl 仓库。Windows 工件包含第三方运行 DLL、许可证、来源与哈希、
源代码归档及构建配方/补丁，但仍需 Microsoft VC++ x64 运行库；Unix 工件使用系统依赖。
另一个原生框架任务显式开启 `WITH_IOS_CONTROLLER`，基于 MaaDeps 编译框架 API、
Toolkit 和控制器会话；主框架该选项暂默认 `OFF`，启用前需提供额外依赖。
CI 离线测试不等于手机或虚拟机的实机兼容验证。
