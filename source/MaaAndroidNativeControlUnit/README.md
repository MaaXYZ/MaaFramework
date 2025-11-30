# MaaAndroidNativeControlUnit

Android 原生控制单元，用于在 Android 设备上直接运行 MaaFramework 时提供截图和输入功能。

## 概述

该模块使用 Android 原生 API（通过 JNI）实现截图和输入注入功能，无需 adb 连接。适用于 MaaFramework 直接在 Android 设备上作为 .so 库运行的场景。

## 使用方法

```cpp
#include <MaaFramework/MaaAPI.h>

// 创建 Android 原生控制器（仅在 Android 设备上可用）
MaaController* controller = MaaAndroidNativeControllerCreate();

// 连接
MaaControllerPostConnection(controller);
MaaControllerWait(controller, id);

// 截图
MaaControllerPostScreencap(controller);

// 点击
MaaControllerPostClick(controller, x, y);

// 销毁（使用通用的 MaaControllerDestroy）
MaaControllerDestroy(controller);
```

## 权限要求

- **截图**: 需要 `android.permission.CAPTURE_VIDEO_OUTPUT` 或通过 MediaProjection API
- **输入注入**: 需要 `android.permission.INJECT_EVENTS` (系统权限)

### 获取权限的方式

1. **系统应用**: 将应用签名为系统应用
2. **Root 权限**: 使用 root 权限运行
3. **Shizuku**: 使用 Shizuku 获取 shell 权限
4. **MediaProjection**: 对于截图，可以使用 MediaProjection API（需要用户授权）

## JNI 集成

该模块通过 JNI 调用 Android Java API。调用方需要：

1. 在 Java 层创建 Helper 类，提供截图和输入注入的方法
2. 通过 JNI 将 Helper 对象传递给 Native 层
3. 确保 JavaVM 已正确初始化

### Java Helper 示例

```java
public class MaaScreencapHelper {
    public Bitmap takeScreenshot() {
        // 使用 SurfaceControl.screenshot() 或 MediaProjection
        // 需要相应权限
    }
}

public class MaaInputHelper {
    public boolean injectTouch(int action, int x, int y, int pointerId) {
        // 使用 InputManager.injectInputEvent()
        // 需要 INJECT_EVENTS 权限
    }
    
    public boolean injectKey(int action, int keyCode) {
        // 使用 InputManager.injectInputEvent()
    }
}
```

## 编译

该模块仅在 Android 平台自动编译：

```bash
# Android 交叉编译
cmake --preset 'NinjaMulti' \
    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-23

cmake --build build --preset 'NinjaMulti - Release'
```

## 与其他控制单元的对比

| 特性 | AdbControlUnit | AndroidNativeControlUnit |
|------|----------------|--------------------------|
| 运行位置 | PC 端 | Android 设备本地 |
| 连接方式 | USB/WiFi adb | 直接调用 |
| 延迟 | 较高（网络传输） | 较低 |
| 权限要求 | adb 调试权限 | 系统/Root 权限 |
| 适用场景 | PC 远程控制 | 设备本地自动化 |

## 限制

1. 仅在 Android 平台可用
2. 需要系统级权限才能实现输入注入
3. 截图功能可能需要 MediaProjection 授权
4. 不支持多设备同时控制（本地控制）
