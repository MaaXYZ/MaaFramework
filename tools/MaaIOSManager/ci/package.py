#!/usr/bin/env python3
"""Collect the independent tool, exact source, dependency recipes and notices."""

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


def copy_tree(source: Path, destination: Path) -> None:
    shutil.copytree(source, destination, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns(".git", "__pycache__", "*.pyc", "CMakeFiles"))


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def windows_dependencies(repository: Path, output: Path) -> dict:
    vcpkg = repository / ".ios-vcpkg"
    installed = vcpkg / "installed"
    triplet = installed / "x64-windows"
    owners = {}
    packages = set()
    for listing in (installed / "vcpkg/info").glob("*.list"):
        package = listing.stem.rsplit("_", 2)[0]
        packages.add(package)
        for line in listing.read_text(encoding="utf-8").splitlines():
            if line.startswith("x64-windows/bin/") and line.lower().endswith(".dll"):
                owners[Path(line).name.lower()] = package
    libraries = []
    for library in sorted((triplet / "bin").glob("*.dll")):
        owner = owners.get(library.name.lower())
        if not owner:
            raise RuntimeError(f"DLL has no installed package ownership: {library.name}")
        if not (triplet / "share" / owner / "copyright").is_file():
            raise RuntimeError(f"Missing bundled dependency license: {owner}")
        shutil.copy2(library, output / "bin" / library.name)
        libraries.append({"file": library.name, "package": owner, "sha256": digest(library)})
    if not libraries:
        raise RuntimeError("Windows artifact has no runtime DLLs")
    copy_tree(triplet / "share", output / "licenses" / "vcpkg-packages")
    copy_tree(triplet / "include", output / "source-materials" / "vcpkg" / "installed-headers")
    sources = output / "source-materials" / "vcpkg"
    for path in ["scripts", "triplets"]:
        copy_tree(vcpkg / path, sources / path)
    (sources / "versions").mkdir(parents=True, exist_ok=True)
    shutil.copy2(vcpkg / "versions/baseline.json", sources / "versions/baseline.json")
    for package in sorted(packages):
        copy_tree(vcpkg / "ports" / package, sources / "ports" / package)
        history = vcpkg / "versions" / (package[0] + "-") / (package + ".json")
        if history.is_file():
            destination = sources / "versions" / history.parent.name / history.name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(history, destination)
    shutil.copy2(vcpkg / "LICENSE.txt", sources / "LICENSE.txt")
    shutil.copy2(repository / "tools/MaaIOSManager/ci/vcpkg.json", sources / "vcpkg.json")
    archives = sources / "upstream-archives"
    archives.mkdir(parents=True)
    for archive in sorted((vcpkg / "downloads").iterdir()):
        if archive.is_file() and archive.name.endswith((".tar.gz", ".tar.xz", ".tar.bz2", ".tgz")):
            shutil.copy2(archive, archives / archive.name)
    if not any("ffmpeg" in archive.name.lower() for archive in archives.iterdir()):
        raise RuntimeError("Missing FFmpeg corresponding source archive")
    configuration = sources / "ffmpeg-build-configuration"
    configuration.mkdir(parents=True)
    for path in (vcpkg / "buildtrees/ffmpeg").rglob("*"):
        if path.is_file() and (path.name in {"config.h", "config.mak", "config.log"} or path.suffix == ".log"):
            relative = path.relative_to(vcpkg / "buildtrees/ffmpeg")
            target = configuration / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    return {
        "vcpkg_commit": subprocess.check_output(["git", "-C", str(vcpkg), "rev-parse", "HEAD"], text=True).strip(),
        "triplet": "x64-windows",
        "runtime_libraries": libraries,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--platform", required=True)
    arguments = parser.parse_args()
    repository = arguments.repository.resolve()
    output = arguments.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "licenses").mkdir(exist_ok=True)
    materials = output / "source-materials"
    copy_tree(repository / "tools/MaaIOSManager", materials / "tools/MaaIOSManager")
    copy_tree(repository / "source/MaaIOSControlUnit/Protocol", materials / "source/MaaIOSControlUnit/Protocol")
    shutil.copy2(repository / "LICENSE.md", materials / "LICENSE.md")
    shutil.copy2(repository / "source/MaaIOSControlUnit/Protocol/LICENSE.Apache-2.0", output / "licenses/LICENSE.Apache-2.0")
    shutil.copy2(repository / "source/MaaIOSControlUnit/Protocol/NOTICE", output / "licenses/NOTICE")
    shutil.copy2(repository / "tools/MaaIOSManager/README.md", output / "README.md")
    shutil.copy2(arguments.build / "CMakeCache.txt", materials / "CMakeCache.txt")
    dependency_sources = arguments.build / "_deps"
    if dependency_sources.is_dir():
        for source in dependency_sources.glob("*-src"):
            if source.is_dir():
                copy_tree(source, materials / "fetched-dependencies" / source.name)
    cache_text = (arguments.build / "CMakeCache.txt").read_text(encoding="utf-8")
    for line in cache_text.splitlines():
        if line.startswith("FETCHCONTENT_SOURCE_DIR_") and "=" in line:
            key, value = line.split("=", 1)
            if value and Path(value).is_dir():
                copy_tree(Path(value), materials / "fetched-dependencies" / key.split(":")[0].lower())
        if line.startswith("nlohmann_json_DIR:") and "=" in line:
            directory = Path(line.split("=", 1)[1]).resolve()
            for prefix in directory.parents:
                header_source = prefix / "include/nlohmann"
                if header_source.is_dir():
                    copy_tree(header_source, materials / "system-headers/nlohmann")
                    for license_file in prefix.glob("LICENSE*"):
                        if license_file.is_file():
                            shutil.copy2(license_file, output / "licenses" / ("nlohmann-json-" + license_file.name))
                    break
    provenance = {
        "commit": subprocess.check_output(["git", "-C", str(repository), "rev-parse", "HEAD"], text=True).strip(),
        "platform": arguments.platform,
        "binary_dependencies": "vcpkg runtime DLLs" if arguments.platform.startswith("windows") else "system package libraries (not bundled)",
    }
    if arguments.platform.startswith("windows"):
        provenance.update(windows_dependencies(repository, output))
        note = "Bundled third-party DLLs are listed in provenance.json. The Microsoft VC++ 2015-2022 x64 runtime is required.\n"
    else:
        note = "This executable uses system OpenSSL, FFmpeg and libcurl packages; it does not bundle those runtime libraries.\n"
    (output / "RUNTIME.txt").write_text(note, encoding="utf-8")
    (output / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    hashes = {str(path.relative_to(output)): digest(path) for path in sorted(output.rglob("*")) if path.is_file()}
    (output / "SHA256SUMS.json").write_text(json.dumps(hashes, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
