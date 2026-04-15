---
name: maa-logging
description: MaaFramework 日志宏用法指南。Use when writing logging code, using LogInfo/LogError/LogWarn/LogDebug/LogTrace, outputting containers or custom types to logs, or when the user asks about logging best practices.
---

# MaaFramework 日志宏用法

头文件：`source/MaaUtils/include/MaaUtils/Logger.h`

## 可用宏

| 宏 | 级别 |
|---|---|
| `LogFatal` / `LogError` / `LogWarn` | 错误与警告 |
| `LogInfo` / `LogDebug` / `LogTrace` | 信息与调试 |
| `LogFunc` | 函数作用域（进入打 enter，离开打 leave + 耗时） |
| `VAR(x)` | 格式化为 `[x=value]` |
| `VAR_VOIDP(x)` | 同上，但将指针转为 `void*` 输出 |

## 核心原则：不要手动拼字符串

用 `<<` 流式输出即可，各片段间自动加空格分隔：

```cpp
// Good
LogInfo << "Screen size:" << width << "x" << height;
LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);

// Bad - 不要手动拼接
LogInfo << "Screen size: " + std::to_string(width) + "x" + std::to_string(height);
LogInfo << std::format("Screen size: {}x{}", width, height);
```

## 容器直接输出

`vector`、`set`、`map<string, T>` 等 STL 容器可以直接 `<<`，会自动序列化为 JSON：

```cpp
std::vector<std::string> names = { "a", "b", "c" };
LogInfo << "names:" << names;       // 输出: names: ["a","b","c"]

std::map<std::string, int> config;
LogInfo << "config:" << config;     // 输出: config: {"key":value,...}
```

不需要手动写 for 循环来打印容器内容。

## 自定义类型输出

给结构体加 `MEO_TOJSON(...)` 或 `MEO_JSONIZATION(...)`，即可直接输出：

```cpp
struct ProcessInfo
{
    os_pid pid = 0;
    std::string name;

    MEO_TOJSON(pid, name);  // 只需序列化，不需要反序列化
};

// MEO_JSONIZATION = MEO_TOJSON + MEO_FROMJSON + MEO_CHECKJSON
struct OCRerResult
{
    std::wstring text;
    cv::Rect box {};
    double score = 0.0;

    MEO_JSONIZATION(text, box, score);
};

// 直接输出，包括容器嵌套也能工作
LogInfo << VAR(results);  // results 是 vector<OCRerResult>
```

## 原理简述

`LogStream::stream` 按优先级尝试：
1. 可构造 `json::value` → `dumps()` 输出（含 `MEO_TOJSON` 类型、基本类型）
2. 可构造 `json::array` → `dumps()` 输出（含序列容器）
3. 可构造 `json::object` → `dumps()` 输出（含 `map<string, T>`）
4. 有 `operator<<` → 直接流输出
5. 以上都不满足 → `static_assert` 编译失败

如果遇到编译报错 "Unsupported type"，给类型加 `MEO_TOJSON` 或特化 `json::ext::jsonization<T>` 即可。
