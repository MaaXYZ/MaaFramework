# 单元与集成测试 / Unit and integration tests

## 中文

测试实际调用本地编译的 MaaFramework，使用内存图像、临时资源和自定义控制器，无需连接设备。
请勿将当前 Python binding 与旧版动态库混用。

```sh
python tools/maadeps-download.py
cmake -S . -B build -DWITH_DBG_CONTROLLER=ON -DWITH_REPLAY_CONTROLLER=ON -DWITH_CUSTOM_CONTROLLER=ON -DWITH_MAA_AGENT=ON -DBUILD_PIPELINE_TESTING=ON
cmake --build build --config Release --parallel
cmake --install build --prefix install --config Release
python -m pip install ./source/binding/Python
python -m pip install -r ./test/requirements.txt
python test/run_tests.py ./source/binding/Python ./install
```

首次构建前请按[构建指南](../docs/zh_cn/4.1-构建指南.md)安装编译器，并初始化子模块。

| 套件 | 覆盖内容 |
| --- | --- |
| `unit` | C API 空句柄/参数校验；Buffer 数据与生命周期；Resource 加载、覆盖和恢复；Controller 参数、截图和失败；Tasker/Context 状态、详情、事件和隔离；Pipeline 算法、字段、归一化、错误输入及资源文件 |
| `binding` | 原有 Python 接口、平台枚举、模板多 Bundle、Command、线程初始化回归 |
| `pipeline` | 原有 Pipeline 解析与实际任务执行、ROI/target、动作、wait_freezes、录制回放 |
| `agent` | IPC 跨进程接口及与本地相同的数据隔离、状态与失败结果契约 |
| `agent-tcp` | TCP、数字标识连接、远程接口，以及超时、断开和故障服务端回归 |

只运行部分套件：

```sh
python test/run_tests.py ./source/binding/Python ./install --suite unit
python test/run_tests.py ./source/binding/Python ./install --suite agent --suite agent-tcp
```

每个套件运行于独立进程；默认每个套件限时 600 秒，可通过 `--timeout` 调整。
某个套件失败后仍继续其他套件，最后列出结果并返回非零退出码。
`unit` 自动发现 `test/python/test_*.py` 中的 `unittest.TestCase`，新增用例无需逐项修改 CI。
析构函数与后台线程中未处理的异常也会使 `unit` 套件失败。
不要在模块导入时运行测试或解析命令行参数；回调内部的断言应通过主线程检查任务结果和完成标记来确认。

现有 C++ Pipeline、动态库加载和 NodeJS 测试仍由 CI 独立运行。
新增 NodeJS 契约测试接入 `test/nodejs/binding.ts`，也可在准备 Node binding 并启用 `WITH_NODEJS_BINDING` 构建后，从 `test/nodejs` 目录运行 `node api-contract.ts`。其同步回调回归在独立子进程中限时执行。
Windows、Linux 和 macOS 的 Python 测试统一调用上述入口；实际设备和各平台输入方式仍需对应环境验证。
仅修改 Schema 时，`schema.yml` 也会独立运行 Schema 契约测试，无需编译本体。

## English

The suites call a locally built MaaFramework using in-memory images, temporary resources and custom controllers; no device is required.
Use native libraries built from the same source revision as the Python binding.
Install the compiler and initialize submodules following the [build guide](../docs/en_us/4.1-BuildGuide.md), then run the build and test commands above.

| Suite | Coverage |
| --- | --- |
| `unit` | C API null handles and argument validation; Buffer contents and ownership; Resource loading, overrides and recovery; Controller parameters, screenshots and failures; Tasker/Context state, details, events and isolation; Pipeline algorithms, fields, normalization, invalid inputs and resource files |
| `binding` | Existing Python API, platform enums, multi-bundle templates, Command and concurrent initialization regressions |
| `pipeline` | Existing Pipeline parsing and task execution, ROI/targets, actions, wait_freezes and recorded replay |
| `agent` | IPC remote interfaces, isolation, state and failure-result contracts |
| `agent-tcp` | TCP and numeric identifiers, remote interfaces, timeout, disconnection and fault-server regressions |

Use repeated `--suite` options to select suites. Each suite runs in a separate process with a configurable `--timeout` of 600 seconds by default.
Failures do not prevent subsequent suites from running; the final summary reports every result and returns a nonzero exit code if any suite failed.
The `unit` suite discovers `unittest.TestCase` classes in `test/python/test_*.py` automatically.
Unhandled destructor and background-thread exceptions also fail the `unit` suite.
Do not run tests or parse command-line arguments when importing a module. Verify callback assertions through task results and completion markers checked by the caller.

CI runs this entry point on Windows, Linux and macOS, alongside the existing C++ Pipeline, dynamic-loading and NodeJS tests.
The new NodeJS contracts run from `test/nodejs/binding.ts`. After preparing the Node binding and building with `WITH_NODEJS_BINDING`, they can also run as `node api-contract.ts` from `test/nodejs`. Synchronous callback regressions use a separate child process with a timeout.
Device behavior and platform-specific input methods still require their corresponding environments.
For schema-only changes, `schema.yml` also runs the schema contracts independently without building native libraries.
