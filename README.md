<!-- markdownlint-disable MD033 MD041 -->
<p align="center">
    <img alt="LOGO" src="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" width="256" height="256" />
</p>

<div align="center">

# MaaFramework

<!-- prettier-ignore-start -->
<!-- markdownlint-disable-next-line MD036 -->
_✨ 基于图像识别的跨平台软件自动化测试框架 ✨_
<!-- prettier-ignore-end -->

</div>

<p align="center">
    <img alt="license" src="https://img.shields.io/github/license/MaaAssistantArknights/MaaFramework">
    <img alt="C++" src="https://img.shields.io/badge/C++-20-%2300599C?logo=cplusplus">
    <img alt="platform" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet">
    <img alt="commit" src="https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaFramework?color=%23ff69b4">
    <img alt="stars" src="https://img.shields.io/github/stars/MaaAssistantArknights/MaaFramework?style=social">
</p>

<div align="center">

[English](./README_en.md) | [简体中文](./README.md)

</div>

## 简介

**MaaFramework** 是基于图像识别技术、运用 [MAA](https://github.com/MaaAssistantArknights/MaaAssistantArknights) 开发经验去芜存菁、完全重写的新一代软件自动化测试框架。

低代码的同时仍拥有高扩展性，旨在打造一款丰富、领先、且实用的开源库，助力开发者轻松编写出更好的测试程序，并推广普及。

## 即刻开始

- [快速开始](docs/zh_cn/1.1-快速开始.md)
- [集成文档](docs/zh_cn/2.1-集成文档.md)
- [回调协议](docs/zh_cn/2.2-回调协议.md)
- [Pipeline 协议](docs/zh_cn/3.1-任务流水线协议.md)

## 最佳实践

- [MAA1999](https://github.com/MaaAssistantArknights/MAA1999) 1999 小助手  
  基于全新架构的 亿韭韭韭 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

- [MAAIP](https://github.com/Arcelibs/MAAIP) 基於 MAA 框架技術的 idolypride 自動化程序  
  由 Github 上最火熱的 lk99 開源套件 MaaFramework 製作

- [MAABH3](https://github.com/MaaAssistantArknights/MAABH3) 《崩坏3》小助手 | A one-click tool for the daily tasks of Honkai Impact.  
  基于全新架构的 蹦蹦蹦 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

- [MAALimbusCompany](https://github.com/hxdnshx/MAALimbusCompany) 边狱公司 小助手  
  基于全新架构的 边狱公司 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

- [MBA](https://github.com/MaaAssistantArknights/MBA) BA 小助手  
  基于 MAA 全新架构的 BA 小助手。图像技术 + 模拟控制，解放双手，不再点点点！由 MaaFramework 强力驱动！

- [MAS](https://github.com/MaaAssistantArknights/MaaAssistantSkland) 森空岛 小助手  
  基于全新架构的 森空岛 小助手。图像技术 + 模拟控制，解放双手！由 MaaFramework 强力驱动！

## 生态共建

MAA 正计划建设为一类项目，而非舟的单一软件。  

若您的项目依赖于 MaaFramework，我们欢迎您将它命名为 MaaXXX, MXA, MAX 等等。当然，这是许可而不是限制，您也可以自由选择其他与 MAA 无关的名字，完全取决于您自己的想法！  

同时，我们也非常欢迎在上方的最佳实践列表中添加上您的项目！

## 许可证

`MaaFramework` 采用 [`LGPL-3.0`](./LICENSE.md) 许可证进行开源。

## 开发

### 本地开发

1. 下载 MaaDeps prebuilt

    ```sh
    python3 tools/maadeps-download.py
    ```

2. _(macOS only)_ 下载 fmt

    ```sh
    sh tools/fetch-fmt.sh
    ```

3. 配置 cmake

    - Windows  

    ```bash
    cmake --preset "MSVC 2022"
    ```

    - Linux / macOS

    ```bash
    cmake --preset "NinjaMulti"
    ```

4. 使用 cmake 构建工程

    ```bash
    cmake --build build --config Release
    cmake --install build --prefix install
    ```

生成的二进制及相关文件在 `install` 目录下

详情请参考 [Build CI](https://github.com/MaaAssistantArknights/MaaFramework/blob/main/.github/workflows/build.yml).

### 在线开发

使用 Codespaces (Dev Container) 一键配置基础开发环境 (Ninja、Cmake、g++-13)：

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://github.com/codespaces/new?hide_repo_select=true&ref=master&repo=632024122)

随后按照 [本地开发](#本地开发) 流程进行配置

## 鸣谢

### 开发者

感谢以下开发者对 MaaFramework 作出的贡献：

<a href="https://github.com/MaaAssistantArknights/MaaFramework/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=MaaAssistantArknights/MaaFramework&max=1000" />
</a>
