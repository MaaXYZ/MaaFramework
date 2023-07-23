<div align="center">

<img alt="LOGO" src="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" width="256" height="256" />

# MAA Framework

<br>

一款软件自动化测试框架，基于图像识别技术，模拟点击控制，一键完成设定好的测试任务

</div>

<br>

## 拆库及解耦

- [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework)  
  技术栈：C++ / Vision  
  通用 图像识别 + 控制 框架，~~Json 解释器~~，不涉及具体待测软件业务逻辑
  目前整体架构已基本完成，更多功能持续更新中！

- [MaaToolKit](https://github.com/MaaAssistantArknights/MaaToolKit)  
  技术栈：C# / RPC / ......  
  平台相关上层业务，~~也就是现在每个 UI 都写了一遍的逻辑~~  
  Http / WS APIs, 模拟器控制、端口查找、定时任务、版本更新、资源下载……
  正在开发中，欢迎加入我们_(:з」∠)_

- MaaCore  
  技术栈：C++ / Python / Vision  
  待测软件业务逻辑，图像识别 + 控制 部分

## 最佳实践

- [MAA1999](https://github.com/MaaAssistantArknights/MAA1999) 《重返未来：1999》小助手  
  基于全新架构的 亿韭韭韭 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

若您的软件集成了 MaaFramework，欢迎向本仓库提交 PR，在列表中添加上您的项目！

## How to build

1. Download pre-built third-party libraries.
  
  ```bash
  python maadeps-download.py
  ```

2. Build with MAA.sln or cmake.
