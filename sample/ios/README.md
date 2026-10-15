# iOS 无边记示例 / iOS Freeform example

## 使用

本示例完全使用流水线，通过 PiCli 运行，不需要 Python、Agent 或自定义动作。
准备启用 iOS 支持的 MaaFramework/PiCli，保持手机解锁，安装无边记并保持竖屏。

在仓库根目录执行（以 macOS `RelWithDebInfo` 构建为例）：

```bash
cp sample/ios/interface.json build/bin/RelWithDebInfo/interface.json
cp -R sample/ios/resource build/bin/RelWithDebInfo/
cd build/bin/RelWithDebInfo
./MaaPiCli
```

命令会更新运行目录中的 interface 和同名资源，请先备份自己的文件或使用独立运行目录。
`config/` 中的配置及配对凭据不会被复制命令覆盖。

### 配对和设备准备

进入 iOS 控制器配置，选择 USB 配对或 Wi-Fi 配对：

- USB：连接并解锁手机，确认设备上的信任或配对提示。
- Wi-Fi：电脑和手机处于同一局域网，在「设置 → 隐私与安全性 → 开发者模式」选择电脑，输入终端显示的一次性配对码。

看到 `Pairing saved to PiCli configuration.` 后，凭据保存在运行目录的
`config/maa_pi_config.json` 的 `ios.pairing_records` 中，下次可选择 `Use existing pairing`。
手机已取消配对时，可在此菜单选择 `Clear all local pairing credentials` 并确认，清除旧凭据后重新配对。
此操作会删除 PiCli 保存的所有设备凭据，不会取消手机上的授权。

PiCli 默认在运行任务前自动准备 DDI：复用可确认适用的已挂载镜像，否则按需下载、安装并验证挂载。
缓存路径、跳过准备及超时选项见 [iOS 管理 API](../../tools/ios/README.md)。

### 任务

- `iOS screenshot check`：获取一帧图像，不点按手机。成功只表示返回非空图像，不判断画面内容。
- `Freeform Apple drawing`：启动无边记，识别并进入绘图模式，用两次连续折线 `Swipe` 绘制果身与叶子，再确认笔迹。该任务会修改画布，默认不选中。

控制器短边缩放固定为 720；模板、界面坐标和轮廓基于约 720×1568 的竖屏画面。
其他机型、方向或系统界面可能需要调整。每条轮廓内不抬手，两条轮廓之间抬手；
轮廓点来自 simple-icons 的 Apple 图标（CC0-1.0）。任务成功表示识别与动作执行成功，不做绘图前后的像素差分验收。

## Usage

This example uses only pipelines and runs in PiCli. Python, agents and custom actions are not required.
Build MaaFramework/PiCli with iOS support, install Freeform and keep the device unlocked in portrait orientation.

Run from the repository root (macOS `RelWithDebInfo` build):

```bash
cp sample/ios/interface.json build/bin/RelWithDebInfo/interface.json
cp -R sample/ios/resource build/bin/RelWithDebInfo/
cd build/bin/RelWithDebInfo
./MaaPiCli
```

Back up existing interface or same-named resource files, or use a separate runtime directory.
The copy commands do not overwrite configuration or credentials in `config/`.

### Pairing and preparation

In the iOS controller configuration, choose USB or Wi-Fi pairing:

- USB: connect and unlock the phone, then accept its trust or pairing prompts.
- Wi-Fi: use the same LAN, select the computer under Settings > Privacy & Security > Developer Mode, and enter the one-use code shown in the terminal.

After `Pairing saved to PiCli configuration.`, credentials are stored in `ios.pairing_records` in
`config/maa_pi_config.json` under the runtime directory. Choose `Use existing pairing` on subsequent runs.
If pairing was revoked on the phone, choose `Clear all local pairing credentials` and confirm before pairing again.
This removes credentials for all devices saved by PiCli; it does not revoke authorization on the phone.

PiCli prepares the DDI before running tasks by default, reusing a confirmed suitable mounted image or
downloading, installing and verifying the required image. See [iOS management APIs](../../tools/ios/README.md)
for cache, preparation skip and timeout options.

### Tasks

- `iOS screenshot check`: captures a frame without tapping the device. Success only means a nonempty image was returned.
- `Freeform Apple drawing`: launches Freeform, recognizes drawing controls, draws the body and leaf using two continuous polyline `Swipe` actions, and confirms the ink. It modifies the canvas and is not selected by default.

The controller scales the short side to 720; templates, UI coordinates and strokes target a portrait image
of approximately 720×1568. Other devices, orientations or OS versions may require adjustment.
Each stroke keeps the finger down through its waypoints, lifting between the two strokes.
Outline points derive from the simple-icons Apple icon (CC0-1.0).
Task success indicates recognition/action completion; it does not compare before/after pixels to verify the drawing.
