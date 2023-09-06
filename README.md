<div align="center">

<img alt="LOGO" src="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" width="256" height="256" />

# MAA Framework

<br>

一款软件自动化测试框架，基于图像识别技术，模拟点击控制，一键完成设定好的测试任务

</div>

<br>

## 拆库及解耦

- [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework/tree/main/include/MaaFramework)  
  技术栈：C++ / Vision  
  通用 图像识别 + 控制 框架，~~Json 解释器~~，不涉及具体待测软件业务逻辑  
  目前整体架构已基本完成，更多功能持续更新中！

- [MaaToolKit](https://github.com/MaaAssistantArknights/MaaFramework/tree/main/include/MaaToolKit)  
  技术栈：C# / RPC / ......  
  平台相关上层业务，~~也就是现在每个 UI 都写了一遍的逻辑~~  
  Http / WS APIs, 模拟器控制、端口查找、定时任务、版本更新、资源下载……  
  正在开发中，欢迎加入我们_(:з」∠)_

- MaaXXX  
  技术栈：取决于您自己！  
  待测软件业务逻辑，Pipeline，自定义识别器、自定义操作等

## 最佳实践

- [MAA1999](https://github.com/MaaAssistantArknights/MAA1999) 1999 小助手  
  基于全新架构的 亿韭韭韭 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

- [MAAIP](https://github.com/Arcelibs/MAAIP) 基於 MAA 框架技術的 idolypride 自動化程序  
  由 Github 上最火熱的 lk99 開源套件 MaaFramework 製作

- [MAABH3](https://github.com/MaaAssistantArknights/MAABH3) 《崩坏3》小助手 | A one-click tool for the daily tasks of Honkai Impact.  
  基于全新架构的 蹦蹦蹦 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

- [MAALimbusCompany](https://github.com/hxdnshx/MAALimbusCompany) 边狱公司 小助手  
  基于全新架构的 边狱公司 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

## 生态共建

MAA 正计划建设为一类项目，而非舟的单一软件。  

若您的项目依赖于 MaaFramework，我们欢迎您将它命名为 MaaXXX, MXA, MAX 等等。当然，这是许可而不是限制，您也可以自由选择其他与 MAA 无关的名字，完全取决于您自己的想法！  

同时，我们也非常欢迎在上方的最佳实践列表中添加上您的项目！

## How to build

1. Download pre-built third-party libraries.
  
    ```bash
    python maadeps-download.py
    ```

2. Build with MAA.sln or cmake.

## 相关资源

- [OCR ONNX 模型](https://github.com/MaaAssistantArknights/MaaCommonAssets/tree/main/OCR)
- [Pipeline 编辑器](https://github.com/MaaAssistantArknights/MaaJsonViewer)
- [控制器代理（已在 release 中内置）](https://github.com/MaaAssistantArknights/MaaAgentBinary)
