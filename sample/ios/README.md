# iOS 无边记示例 / iOS Freeform example

使用 iOS 控制器启动无边记，通过模板识别进入绘图模式，再用自定义触摸动作绘制苹果轮廓。
脚本只加载本目录的 `resource`，不依赖公共示例资源。

## 使用

准备 MaaFramework Python 绑定及原生库，完成设备配对和 DDI 准备，保持手机解锁并安装无边记。

```bash
python sample/ios/freeform_apple.py --udid <UDID> --pairing-record <credential-file>
python sample/ios/freeform_apple.py --wifi auto --pairing-record <credential-file>
```

凭据文件内容为 Toolkit 配对接口返回的不透明字符串，不是 PiCli 的完整配置 JSON。
此示例会操作手机并修改无边记画布。模板与坐标基于特定竖屏界面，其他机型、方向或系统版本可能需要调整。

## Usage

Install the MaaFramework Python bindings and native libraries, pair the device and prepare its DDI.
Keep the device unlocked with Freeform installed, then run either command above.
The credential file contains the opaque string returned by the Toolkit pairing API, rather than the entire PiCli configuration JSON.

The script launches Freeform, recognizes drawing controls and draws an Apple outline with custom touch actions.
It loads only its local `resource` directory. It modifies the device canvas; templates and coordinates target a specific portrait UI and may need adjustment for other devices or OS versions.
