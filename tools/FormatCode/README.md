# format_cpp.py - C++ 代码格式化工具

## 简介

`format_cpp.py` 是一个用于自动格式化项目中 C++ 文件的 Python 脚本。它使用 `clang-format` 工具，根据项目根目录下的 `.clang-format` 配置文件，对整个项目中的 C++ 文件进行格式化，同时自动排除第三方库目录（如 `3rdparty`）。

## 功能特性

- ✅ **自动查找 C++ 文件**：支持多种 C++ 文件扩展名（`.cpp`, `.hpp`, `.h`, `.cc`, `.cxx`, `.c++`, `.h++`, `.hh`, `.hxx`, `.cppm`）
- ✅ **智能排除目录**：默认排除 `3rdparty` 目录及其子目录
- ✅ **使用项目配置**：自动使用项目根目录下的 `.clang-format` 配置文件
- ✅ **检查模式**：支持 `--dry-run` 模式，只检查格式而不修改文件
- ✅ **详细输出**：支持 `--verbose` 模式，显示详细的处理信息
- ✅ **跨平台支持**：支持 Windows、Linux、macOS

## 前置要求

1. **Python 3.6+**
2. **clang-format**：需要安装 `clang-format` 并在系统 PATH 中可用

### 安装 clang-format

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install clang-format
```

**macOS:**

```bash
brew install clang-format
```

**Windows:**

- 下载 LLVM 安装包：<https://llvm.org/builds/>
- 或使用 Chocolatey: `choco install llvm`

## 使用方法

### 基本用法

```bash
# 格式化所有 C++ 文件（排除 3rdparty 目录）
python3 tools/FormatCode/format_cpp.py

# 只检查格式，不修改文件（用于 CI/CD）
python3 tools/FormatCode/format_cpp.py --dry-run

# 显示详细信息
python3 tools/FormatCode/format_cpp.py --verbose
```

### 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--dry-run` | 只检查格式，不修改文件。如果发现格式问题，退出码为 1 | `False` |
| `--clang-format` | 指定 clang-format 命令名称 | `clang-format` |
| `--exclude` | 要排除的目录列表（可指定多个） | `3rdparty` |
| `--root` | 项目根目录路径 | 脚本所在目录的父目录 |
| `--verbose` | 显示详细的处理信息 | `False` |
| `-h, --help` | 显示帮助信息 | - |

## 使用示例

### 示例 1：格式化所有文件

```bash
python3 tools/FormatCode/format_cpp.py
```

输出：

```text
正在查找 C++ 文件...
找到 150 个 C++ 文件

格式化完成: 150 个成功, 0 个失败
```

### 示例 2：检查格式（不修改）

```bash
python3 tools/FormatCode/format_cpp.py --dry-run
```

输出：

```text
正在查找 C++ 文件...
找到 150 个 C++ 文件

[1/150] source/MaaFramework/Task.cpp... ✗ 需要格式化
[2/150] source/MaaFramework/Resource.cpp... ✓
...

检查完成: 150 个文件
需要格式化的文件: 5
```

### 示例 3：使用特定版本的 clang-format

```bash
python3 tools/FormatCode/format_cpp.py --clang-format clang-format-17
```

### 示例 4：排除多个目录

```bash
python3 tools/FormatCode/format_cpp.py --exclude 3rdparty build install
```

### 示例 5：指定项目根目录

```bash
python3 tools/FormatCode/format_cpp.py --root /path/to/project
```

### 示例 6：详细模式

```bash
python3 tools/FormatCode/format_cpp.py --verbose
```

输出：

```text
项目根目录: /path/to/MaaFramework
配置文件: /path/to/MaaFramework/.clang-format
排除目录: ['3rdparty']
clang-format 命令: clang-format

正在查找 C++ 文件...
找到 150 个 C++ 文件

[1/150] source/MaaFramework/Task.cpp... ✓ 格式化成功
[2/150] source/MaaFramework/Resource.cpp... ✓ 格式化成功
...
```

## 与 CI/CD 集成

### GitHub Actions

项目已配置 GitHub Actions 工作流，可以自动格式化和检查代码：

1. **手动格式化**：在 GitHub Actions 中运行 "Format Code" 工作流，会自动格式化所有代码并提交
2. **PR 检查**：创建 Pull Request 时，会自动检查修改的 C++ 文件格式是否正确

### 在 CI 中使用

```yaml
- name: Check code format
  run: |
    python3 tools/FormatCode/format_cpp.py --dry-run
```

如果格式不正确，脚本会以退出码 1 退出，导致 CI 失败。

## 工作原理

1. **查找文件**：递归遍历项目根目录，查找所有 C++ 文件
2. **过滤排除**：排除指定目录（默认 `3rdparty`）及其子目录中的文件
3. **检查配置**：验证项目根目录下是否存在 `.clang-format` 配置文件
4. **格式化/检查**：
   - 正常模式：使用 `clang-format -i` 直接修改文件
   - 检查模式：使用 `clang-format --dry-run --Werror` 检查格式

## 支持的 C++ 文件扩展名

脚本支持以下 C++ 文件扩展名：

- `.cpp` - C++ 源文件
- `.hpp` - C++ 头文件
- `.h` - C/C++ 头文件
- `.cc` - C++ 源文件
- `.cxx` - C++ 源文件
- `.c++` - C++ 源文件
- `.h++` - C++ 头文件
- `.hh` - C++ 头文件
- `.hxx` - C++ 头文件
- `.cppm` - C++ 模块文件

## 常见问题

### Q: 脚本找不到 clang-format

**A:** 确保 `clang-format` 已安装并在系统 PATH 中。可以使用 `--clang-format` 参数指定完整路径：

```bash
python3 tools/FormatCode/format_cpp.py --clang-format /usr/local/bin/clang-format
```

### Q: 如何只格式化特定目录？

**A:** 可以使用 `--root` 参数指定目录，或修改脚本的排除逻辑。目前脚本设计为格式化整个项目。

### Q: 脚本提示找不到 .clang-format 文件

**A:** 确保项目根目录下存在 `.clang-format` 配置文件。脚本会自动检测项目根目录（脚本所在目录的父目录）。

### Q: 如何排除更多目录？

**A:** 使用 `--exclude` 参数指定多个目录：

```bash
python3 tools/FormatCode/format_cpp.py --exclude 3rdparty build install
```

### Q: 在 Windows 上如何使用？

**A:** 在 Windows 上，可以使用 PowerShell 或 CMD：

```powershell
python tools\FormatCode\format_cpp.py
```

或使用 Git Bash：

```bash
python3 tools/FormatCode/format_cpp.py
```

## 退出码

- `0`：成功（所有文件格式正确或格式化成功）
- `1`：失败（在 `--dry-run` 模式下发现格式问题，或格式化过程中出错）

## 注意事项

1. **备份代码**：虽然脚本会保留原始格式，但建议在格式化前提交代码或创建备份
2. **第三方库**：默认排除 `3rdparty` 目录，不要格式化第三方库代码
3. **配置文件**：确保 `.clang-format` 配置文件符合项目规范
4. **版本兼容**：不同版本的 `clang-format` 可能有不同的格式化结果，建议团队使用相同版本

## 相关文件

- `.clang-format` - clang-format 配置文件（位于项目根目录）
- `.github/workflows/format.yml` - 自动格式化工作流
- `.github/workflows/format-check.yml` - PR 格式检查工作流

## 贡献

如果发现问题或有改进建议，请提交 Issue 或 Pull Request。
