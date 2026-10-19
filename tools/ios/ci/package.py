#!/usr/bin/env python3
"""Collect the iOS framework runtime, optional source bundle, dependency recipes and notices."""

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path
from typing import Optional


def copy_tree(source: Path, destination: Path) -> None:
    shutil.copytree(source, destination, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns(
                        ".git", "__pycache__", "*.pyc", "CMakeFiles", "MaaDeps", "node_modules",
                        ".pnpm-store", ".venv", "venv", ".cache", "build", "build-*", "TestingDataSet",
                        "CMakeCache.txt", "*.o", "*.obj", "*.node"))


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def relocate_runtime(output: Path, prefix: Path, libraries: list[Path], system: str) -> None:
    if system == "linux":
        for library in libraries:
            subprocess.run(["patchelf", "--set-rpath", "$ORIGIN", str(library)], check=True)
        return
    if system != "macos":
        return
    for binary in sorted((output / "bin").iterdir()):
        if not binary.is_file() or binary.suffix not in {"", ".dylib", ".node"}:
            continue
        listing = subprocess.run(["otool", "-L", str(binary)], capture_output=True, text=True)
        if listing.returncode:
            continue
        changes = []
        for line in listing.stdout.splitlines()[1:]:
            dependency = line.strip().split(" (", 1)[0]
            if dependency.startswith(str(prefix) + "/"):
                name = Path(dependency).name
                if not (output / "bin" / name).is_file():
                    raise RuntimeError(f"Unbundled runtime dependency: {dependency}")
                changes += ["-change", dependency, f"@rpath/{name}"]
        if binary in libraries:
            changes += ["-id", f"@rpath/{binary.name}"]
            load_commands = subprocess.check_output(["otool", "-l", str(binary)], text=True)
            if "path @loader_path (" not in load_commands:
                changes += ["-add_rpath", "@loader_path"]
        if changes:
            subprocess.run(["install_name_tool", *changes, str(binary)], check=True)
            subprocess.run(["codesign", "--force", "--sign", "-", str(binary)], check=True)


def dependencies(arguments, output: Path, source_output: Optional[Path]) -> dict:
    vcpkg = arguments.vcpkg.resolve()
    installed = vcpkg / "installed"
    prefix = installed / arguments.triplet
    system = arguments.platform.split("-", 1)[0]
    directory = "debug/bin" if arguments.configuration == "Debug" else "bin"
    if system != "windows":
        directory = "lib"
    owners = {}
    packages = set()
    for listing in (installed / "vcpkg/info").glob(f"*_{arguments.triplet}.list"):
        package = listing.stem.rsplit("_", 2)[0]
        packages.add(package)
        for line in listing.read_text(encoding="utf-8").splitlines():
            owners[line] = package
    libraries = []
    runtime = []
    (output / "bin").mkdir(exist_ok=True)
    for library in sorted((prefix / directory).iterdir()):
        if not library.is_file() or not (library.name.endswith((".dll", ".dylib")) or ".so" in library.name):
            continue
        relative = library.relative_to(installed).as_posix()
        owner = owners.get(relative)
        if not owner:
            raise RuntimeError(f"Runtime library has no installed package ownership: {relative}")
        destination = output / "bin" / library.name
        shutil.copy2(library, destination)
        libraries.append(destination)
        runtime.append({"file": library.name, "package": owner})
    if not libraries:
        raise RuntimeError("No iOS runtime dependencies were packaged")
    required = {"openssl", "ffmpeg", "pugixml", "zlib"}
    if system != "macos":
        required.add("curl")
    for package in sorted(packages):
        share = prefix / "share" / package
        license_file = share / "copyright"
        if not license_file.is_file():
            raise RuntimeError(f"Missing dependency license: {package}")
        licenses = output / "bin/licenses/ios" / package
        licenses.mkdir(parents=True, exist_ok=True)
        shutil.copy2(license_file, licenses / "copyright")
        if source_output is not None:
            materials = share / "source-materials"
            if package in required and not (materials / "source.tar.gz").is_file():
                raise RuntimeError(f"Missing corresponding sources: {package}")
            if materials.is_dir():
                copy_tree(materials, source_output / "vcpkg/dependencies" / package)
    if not required.issubset(packages):
        raise RuntimeError(f"Missing installed dependencies: {required - packages}")
    if source_output is not None:
        sources = source_output / "vcpkg"
        for directory_name in ["scripts", "triplets"]:
            copy_tree(vcpkg / directory_name, sources / directory_name)
        copy_tree(prefix / "include", sources / "installed-headers")
        shutil.copy2(vcpkg / "LICENSE.txt", sources / "LICENSE.txt")
        shutil.copy2(arguments.dependency_config, sources / "framework.cmake")
        copy_tree(arguments.dependency_config.parent / "triplets", sources / "overlay-triplets")
    relocate_runtime(output, prefix, libraries, system)
    for library in runtime:
        library["sha256"] = digest(output / "bin" / library["file"])
    return {
        "vcpkg_commit": subprocess.check_output(["git", "-C", str(vcpkg), "rev-parse", "HEAD"], text=True).strip(),
        "triplet": arguments.triplet,
        "configuration": arguments.configuration,
        "runtime_libraries": runtime,
    }


def collect_source_materials(repository: Path, build: Path, materials: Path) -> None:
    for directory in ["source", "include", "3rdparty", "cmake", "tools", "test"]:
        source = repository / directory
        if not source.is_dir():
            raise RuntimeError(f"Framework source directory is missing: {directory}")
        copy_tree(source, materials / directory)
    for filename in ["CMakeLists.txt", "CMakePresets.json", "LICENSE.md"]:
        shutil.copy2(repository / filename, materials / filename)
    recipes = materials / "build-recipes"
    recipes.mkdir(parents=True, exist_ok=True)
    for name in ["build.yml", "test.yml"]:
        shutil.copy2(repository / ".github/workflows" / name, recipes / name)
    copy_tree(repository / ".github/actions/ios-dependencies", recipes / "ios-dependencies")
    maadeps = repository / "source/MaaUtils/MaaDeps"
    maadeps_source = materials / "source/MaaUtils/MaaDeps"
    maadeps_source.mkdir(parents=True, exist_ok=True)
    shutil.copy2(maadeps / "maadeps.cmake", maadeps_source / "maadeps.cmake")
    dependency_sources = build / "_deps"
    if dependency_sources.is_dir():
        for source in dependency_sources.glob("*-src"):
            if source.is_dir():
                copy_tree(source, materials / "fetched-dependencies" / source.name)
    cache_file = build / "CMakeCache.txt"
    if cache_file.is_file():
        cache_text = cache_file.read_text(encoding="utf-8")
        for line in cache_text.splitlines():
            if line.startswith("FETCHCONTENT_SOURCE_DIR_") and "=" in line:
                key, value = line.split("=", 1)
                if value and Path(value).is_dir():
                    copy_tree(Path(value), materials / "fetched-dependencies" / key.split(":")[0].lower())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-output", type=Path, default=None)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--vcpkg", type=Path, required=True)
    parser.add_argument("--triplet", required=True)
    parser.add_argument("--configuration", choices=("Debug", "Release", "RelWithDebInfo"), required=True)
    parser.add_argument("--dependency-config", type=Path, required=True)
    arguments = parser.parse_args()
    repository = arguments.repository.resolve()
    output = arguments.output.resolve()
    materials = arguments.source_output.resolve() if arguments.source_output is not None else None
    if materials is not None and (output == materials or output in materials.parents or materials in output.parents):
        raise RuntimeError("Binary and source output directories must not overlap")
    if (output / "source-materials").exists():
        raise RuntimeError("Remove legacy source-materials from the binary output before packaging")
    output.mkdir(parents=True, exist_ok=True)
    if materials is not None:
        collect_source_materials(repository, arguments.build, materials)
    source_artifact = f"ios-source-{arguments.platform}"
    provenance = {
        "commit": subprocess.check_output(["git", "-C", str(repository), "rev-parse", "HEAD"], text=True).strip(),
        "platform": arguments.platform,
        "binary_dependencies": "MaaFramework/MaaDeps runtime and target-built iOS protocol dependencies",
        "maadeps_version": "v3.0.0",
        "submodules": subprocess.check_output(["git", "-C", str(repository), "submodule", "status", "--recursive"], text=True).splitlines(),
        "source_artifact": source_artifact,
    }
    provenance.update(dependencies(arguments, output, materials))
    ios_license_dir = output / "bin/licenses/ios"
    ios_license_dir.mkdir(parents=True, exist_ok=True)
    note = ("iOS protocol runtime dependencies are listed in bin/licenses/ios/provenance.json.\n"
            f"Corresponding sources and build recipes are distributed separately as {source_artifact}-<version>.zip.\n")
    if arguments.platform.startswith("windows"):
        note += "The Microsoft Visual C++ 2015-2022 runtime for the target architecture is required.\n"
    (ios_license_dir / "RUNTIME.txt").write_text(note, encoding="utf-8")
    (ios_license_dir / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    if materials is not None:
        (materials / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
        hashes = {str(path.relative_to(materials)): digest(path) for path in sorted(materials.rglob("*"))
                  if path.is_file() and path.name != "SHA256SUMS.json"}
        (materials / "SHA256SUMS.json").write_text(json.dumps(hashes, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
