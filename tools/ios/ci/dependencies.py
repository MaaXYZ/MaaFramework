#!/usr/bin/env python3
"""Build iOS host dependencies with the same target toolchain as MaaFramework."""

import argparse
import os
import platform
import subprocess
from pathlib import Path


def cmake_path(path):
    return str(path.resolve()).replace("\\", "/")


def configure(repository, vcpkg, work, system, architecture):
    arch = "x64" if architecture == "x86_64" else "arm64"
    target = "osx" if system == "macos" else system
    triplet = f"maa-ios-{arch}-{target}"
    overlays = work / "triplets"
    overlays.mkdir(parents=True, exist_ok=True)
    lines = [f"set(VCPKG_TARGET_ARCHITECTURE {arch})",
             "set(VCPKG_CRT_LINKAGE dynamic)",
             "set(VCPKG_LIBRARY_LINKAGE static)",
             'if(PORT STREQUAL "ffmpeg")',
             "    set(VCPKG_LIBRARY_LINKAGE dynamic)", "endif()"]
    chainload = None
    if system == "windows":
        lines += ["set(VCPKG_LIBRARY_LINKAGE dynamic)"]
    else:
        lines += ["set(VCPKG_BUILD_TYPE release)"]
    if system == "linux":
        deps = repository / "source/MaaUtils/MaaDeps"
        chainload = deps / f"cmake/maa-{arch}-linux-toolchain.cmake"
        triple = "x86_64-linux-gnu" if arch == "x64" else "aarch64-linux-gnu"
        gcc = cmake_path(deps / "x-tools" / triple)
        lines += ['set(VCPKG_CMAKE_SYSTEM_NAME Linux)',
                  f'set(VCPKG_MAKE_BUILD_TRIPLET "--host={triple}")',
                  f'set(VCPKG_C_FLAGS "--target={triple} -pthread")',
                  f'set(VCPKG_CXX_FLAGS "--target={triple} -pthread")',
                  f'set(VCPKG_LINKER_FLAGS "--target={triple} --gcc-toolchain={gcc} -pthread")']
    elif system == "macos":
        lines += ['set(VCPKG_CMAKE_SYSTEM_NAME Darwin)',
                  f'set(VCPKG_OSX_ARCHITECTURES {"x86_64" if arch == "x64" else "arm64"})',
                  'set(VCPKG_OSX_DEPLOYMENT_TARGET 13.3)']
    elif system == "android":
        ndk = os.environ.get("ANDROID_NDK_ROOT")
        if not ndk:
            raise RuntimeError("ANDROID_NDK_ROOT is required")
        chainload = Path(ndk) / "build/cmake/android.toolchain.cmake"
        abi = "x86_64" if arch == "x64" else "arm64-v8a"
        lines += ['set(VCPKG_CMAKE_SYSTEM_NAME Android)',
                  'set(VCPKG_CMAKE_SYSTEM_VERSION 24)',
                  f'set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DANDROID_ABI={abi} -DANDROID_PLATFORM=android-24)']
    if chainload:
        if not chainload.is_file():
            raise FileNotFoundError(chainload)
        lines += [f'set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "{cmake_path(chainload)}")']
    hook = repository / "tools/ios/ci/source-materials.cmake"
    lines += [f'list(APPEND Z_VCPKG_POST_PORTFILE_INCLUDES "{cmake_path(hook)}")',
              f'list(APPEND VCPKG_HASH_ADDITIONAL_FILES "{cmake_path(hook)}")']
    (overlays / f"{triplet}.cmake").write_text("\n".join(lines) + "\n", encoding="utf-8")

    prefix = vcpkg / "installed" / triplet
    config = work / "framework.cmake"
    initial = {
        "CMAKE_TOOLCHAIN_FILE": vcpkg / "scripts/buildsystems/vcpkg.cmake",
        "VCPKG_INSTALLED_DIR": vcpkg / "installed",
        "VCPKG_TARGET_TRIPLET": triplet,
        "VCPKG_OVERLAY_TRIPLETS": overlays,
        "VCPKG_MANIFEST_MODE": "OFF",
        "OPENSSL_ROOT_DIR": prefix,
        "MAA_IOS_DEPENDENCIES_ROOT": prefix,
    }
    if chainload:
        initial["VCPKG_CHAINLOAD_TOOLCHAIN_FILE"] = chainload
    if system == "android":
        initial["ANDROID_PLATFORM"] = "android-24"
    config.write_text("\n".join(
        f'set({key} "{cmake_path(value) if isinstance(value, Path) else value}" CACHE STRING "iOS host dependency configuration" FORCE)'
        for key, value in initial.items()) + "\n", encoding="utf-8")
    return triplet, config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument("--vcpkg", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--platform", choices=("windows", "linux", "macos", "android"), required=True)
    parser.add_argument("--architecture", choices=("x86_64", "aarch64"), required=True)
    parser.add_argument("--configure-only", action="store_true")
    args = parser.parse_args()
    repository, vcpkg, work = args.repository.resolve(), args.vcpkg.resolve(), args.work.resolve()
    triplet, config = configure(repository, vcpkg, work, args.platform, args.architecture)
    if not args.configure_only:
        executable = vcpkg / ("vcpkg.exe" if os.name == "nt" else "vcpkg")
        host_arch = "arm64" if platform.machine().lower() in ("arm64", "aarch64") else "x64"
        host_system = {"Windows": "windows", "Darwin": "osx", "Linux": "linux"}[platform.system()]
        subprocess.run([str(executable), "install", "--triplet", triplet,
                        "--host-triplet", f"{host_arch}-{host_system}",
                        f"--overlay-triplets={work / 'triplets'}",
                        f"--x-manifest-root={repository / 'tools/ios/ci'}",
                        f"--x-install-root={vcpkg / 'installed'}"], check=True)
    values = {"MAA_IOS_CMAKE": cmake_path(config), "MAA_IOS_TRIPLET": triplet,
              "MAA_IOS_VCPKG": cmake_path(vcpkg)}
    if os.environ.get("GITHUB_ENV"):
        with open(os.environ["GITHUB_ENV"], "a", encoding="utf-8") as stream:
            stream.write("".join(f"{key}={value}\n" for key, value in values.items()))
    if os.environ.get("GITHUB_PATH") and args.platform == "windows":
        prefix = vcpkg / "installed" / triplet
        with open(os.environ["GITHUB_PATH"], "a", encoding="utf-8") as stream:
            stream.write(f"{prefix / 'debug/bin'}\n{prefix / 'bin'}\n")
    print(f"CMake initial cache: {config}")


if __name__ == "__main__":
    main()
