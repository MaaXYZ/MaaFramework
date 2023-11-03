<!-- markdownlint-disable MD033 MD041 -->
<p align="center">
    <img alt="LOGO" src="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" width="256" height="256" />
</p>

<div align="center">

# MaaFramework

<!-- prettier-ignore-start -->
<!-- markdownlint-disable-next-line MD036 -->
_✨ A cross-platform software automation testing framework based on image recognition ✨_
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

## Introduction

**MaaFramework** is a next-generation software automation testing framework, completely rewritten and refined based on image recognition technology and the development experience of [MAA](https://github.com/MaaAssistantArknights/MaaAssistantArknights).

It offers low-code simplicity while maintaining high extensibility. The framework aims to create a rich, leading-edge, and practical open-source library, empowering developers to easily write better test programs and promoting widespread use.

## Get Started

- [Quick Start](docs/en_us/1.1-QuickStarted.md)
- [Integration](docs/en_us/2.1-Integration.md)
- [Callback Protocol](docs/en_us/2.2-CallbackProtocol.md)
- [Pipeline Protocol](docs/en_us/3.1-PipelineProtocol.md)

## Best Practices

- [MAA1999](https://github.com/MaaAssistantArknights/MAA1999) 1999 Assistant  
  The 1999 Assistant, based on a brand new architecture. Image technology + simulation control, freeing your hands! Powered by MaaFramework.

- [MAAIP](https://github.com/Arcelibs/MAAIP) idolypride Automation Program Based on MAA Framework Technology  
  Created by the hottest open-source package on Github, lk99, using MaaFramework.

- [MAABH3](https://github.com/MaaAssistantArknights/MAABH3) Honkai Impact 3 Assistant  
  A one-click tool for the daily tasks of Honkai Impact, based on a brand new architecture. Image technology + simulation control, freeing your hands! Powered by MaaFramework.

- [MAALimbusCompany](https://github.com/hxdnshx/MAALimbusCompany) Limbus Company Assistant  
  Based on a brand new architecture. Image technology + simulation control, freeing your hands! Powered by MaaFramework.

- [MBA](https://github.com/MaaAssistantArknights/MBA) BA Assistant  
  A BA Assistant based on MAA's new architecture. Image technology + simulation control, no more clicking! Powered by MaaFramework.

- [MAS](https://github.com/MaaAssistantArknights/MaaAssistantSkland) Skland Assistant  
  Based on a brand new architecture. Image technology + simulation control, freeing your hands! Powered by MaaFramework.

## Eco-Building

MAA is planned to be a category of projects rather than just a single piece of software.

If your project depends on MaaFramework, we welcome you to name it as MaaXXX, MXA, MAX, and so on. Of course, this is permission, not limitation. You can freely choose other names unrelated to MAA, depending on your own ideas!

We also welcome adding your project to the best practices list above!

## License

`MaaFramework` is open-sourced under the [`LGPL-3.0`](./LICENSE.md) license.

## Development

### Local Development

1. Download MaaDeps prebuilt

    ```sh
    python3 tools/maadeps-download.py
    ```

2. _(macOS only)_ Download fmt

    ```sh
    sh tools/fetch-fmt.sh
    ```

3. Configure cmake

    - Windows

    ```bash
    cmake --preset "MSVC 2022"
    ```

    - Linux / macOS

    ```bash
    cmake --preset "NinjaMulti"
    ```

4. Build the project using cmake

    ```bash
    cmake --build build --config Release
    cmake --install build --prefix install
    ```

The generated binaries and related files are located in the `install` directory.

For more details, refer to [Build CI](https://github.com/MaaAssistantArknights/MaaFramework/blob/main/.github/workflows/build.yml).

### Online Development

Use Codespaces (Dev Container) to set up the basic development environment with one click (Ninja, CMake, g++-13):

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://github.com/codespaces/new?hide_repo_select=true&ref=master&repo=632024122)

Then follow the steps in [Local Development](#local-development).

## Acknowledgments

### Developers

Thanks to the following developers for their contributions to MaaFramework:

<a href="https://github.com/MaaAssistantArknights/MaaFramework/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=MaaAssistantArknights/MaaFramework&max=1000" />
</a>
