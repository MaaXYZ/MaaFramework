# iOS 无边记示例 / iOS Freeform example

使用 iOS 控制器启动无边记，通过模板识别进入绘图模式，再用自定义触摸动作绘制苹果轮廓。
脚本只加载本目录的 `resource`，不依赖公共示例资源。

## 使用

准备 MaaFramework Python 绑定及原生库，完成 DDI 准备，保持手机解锁并安装无边记。
PiCli 运行 iOS 任务时会自动准备 DDI；本 Python 示例不自动调用准备接口，仅在 PiCli 中配对还不够。

### 配对

在 PiCli 的运行目录执行 `./MaaPiCli`。该目录需要有效的 `interface.json`，其中声明
`type: "IOS"` 的控制器。进入 iOS 控制器配置后选择 USB 配对或 Wi-Fi 配对：

- USB：连接并解锁手机，确认设备上的信任或配对提示。
- Wi-Fi：电脑和手机处于同一局域网，在手机「设置 → 隐私与安全性 → 开发者模式」
  选择电脑，输入终端显示的一次性配对码。

看到 `Pairing saved to PiCli configuration.` 后，凭据已保存到运行目录下
`config/maa_pi_config.json` 的 `ios.pairing_records` 中。下次可选择 `Use existing pairing`。
配对成功即会保存，不需要运行任务。

### 使用 PiCli 保存的凭据运行

以下命令从仓库根目录执行，以 macOS 的 `RelWithDebInfo` 构建目录为例。
先将第一条凭据导出为示例接受的文件；如果配置中有多个设备，请选择对应的数组元素。

```bash
python3 - <<'PYCODE'
import json
from pathlib import Path

build = Path("build/bin/RelWithDebInfo")
config = json.loads((build / "config/maa_pi_config.json").read_text(encoding="utf-8"))
Path("build/ios-example.pair").write_text(config["ios"]["pairing_records"][0], encoding="utf-8")
PYCODE

export MAAFW_BINARY_PATH="$PWD/build/bin/RelWithDebInfo"
export PYTHONPATH="$PWD/source/binding/Python${PYTHONPATH:+:$PYTHONPATH}"
python3 sample/ios/freeform_apple.py --wifi auto --pairing-record build/ios-example.pair
```

`MAAFW_BINARY_PATH` 必须在导入 `maa` 前设置；请根据实际构建目录调整。
示例不会自行读取 PiCli 配置，也不会发起配对；上面的导出步骤将两者连接起来。

### 已有独立凭据文件


```bash
python sample/ios/freeform_apple.py --udid <UDID> --pairing-record <credential-file>
python sample/ios/freeform_apple.py --wifi auto --pairing-record <credential-file>
```

凭据文件内容为 Toolkit 配对接口返回的不透明字符串，不是 PiCli 的完整配置 JSON。
此示例会操作手机并修改无边记画布。模板与坐标基于特定竖屏界面，其他机型、方向或系统版本可能需要调整。

## Usage

Install the MaaFramework Python bindings and native libraries, prepare the DDI and keep the device unlocked with Freeform installed.
PiCli prepares the DDI when running iOS tasks. This Python example does not call preparation automatically; pairing in PiCli alone is insufficient.

### Pairing and running

Run `./MaaPiCli` in its runtime directory, which must contain an `interface.json` declaring an
`IOS` controller. In the iOS controller configuration, choose USB or Wi-Fi pairing:

- USB: connect and unlock the device, then accept its trust or pairing prompts.
- Wi-Fi: use the same LAN, select the computer under Settings > Privacy & Security > Developer Mode,
  then enter the one-use code shown in the terminal.

After `Pairing saved to PiCli configuration.`, credentials are stored in `ios.pairing_records` in
`config/maa_pi_config.json` under the runtime directory. No task execution is needed to save them.
Choose `Use existing pairing` on subsequent runs.

The export-and-run commands above use the first saved credential and the macOS `RelWithDebInfo`
build directory. Run them from the repository root; select the appropriate array entry for other devices.
Set `MAAFW_BINARY_PATH` before importing `maa` and adjust it for your build directory.
The script does not read PiCli configuration or initiate pairing itself.
If you already have an independent credential file, use either shorter command above.
The credential file contains the opaque string returned by the Toolkit pairing API, rather than the entire PiCli configuration JSON.

The script launches Freeform, recognizes drawing controls and draws an Apple outline with custom touch actions.
It loads only its local `resource` directory. It modifies the device canvas; templates and coordinates target a specific portrait UI and may need adjustment for other devices or OS versions.
