from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

import tomlkit

SRC_DIR = Path("source/binding/Python")
PACK_DIR = Path("pip_pack")
WHL_DIR = PACK_DIR / "wheel"
DIST_DIR = PACK_DIR / "dist"


def copy_base():
    print("Copying base...", end="")

    if not PACK_DIR.exists():
        shutil.copytree(SRC_DIR, PACK_DIR)
        with (PACK_DIR / ".gitignore").open("w") as f:
            f.write("*")

    shutil.copy("README.md", PACK_DIR)
    shutil.copy("README_en.md", PACK_DIR)
    shutil.copy("LICENSE.md", PACK_DIR)

    print("done")


def set_toml_ver():
    git_tag = sys.argv[1]
    if "ci" in git_tag:
        base_ver, ci_ver = git_tag.split("ci")
        version = base_ver + "dev" + ci_ver.split("-")[0]
    else:
        version = git_tag

    print(f"Setting version to: {version}")

    toml_path = PACK_DIR / "pyproject.toml"
    with toml_path.open() as f:
        py_project = tomlkit.load(f)
        py_project["project"]["version"] = version

    with toml_path.open("w") as f:
        tomlkit.dump(py_project, f)


def gen_base_whl():
    print("Generating base wheel...", end="")

    subprocess.run(
        [sys.executable, "-m", "build", "-w"],
        cwd=PACK_DIR,
        check=True,
    )

    print("done")


def unpack_base_whl() -> tuple[Path, Path]:
    print("Unpacking base wheel...", end="")

    whl_path = next(DIST_DIR.glob("*.whl"))
    subprocess.run(
        [sys.executable, "-m", "wheel", "unpack", whl_path.name],
        cwd=DIST_DIR,
        check=True,
    )

    # Pick the first matching obj from the generator
    unpack_dir = next(d for d in DIST_DIR.iterdir() if d.is_dir())
    info_dir = next(d for d in unpack_dir.iterdir() if d.suffix == ".dist-info")

    (info_dir / "RECORD").unlink()

    print("done")
    return unpack_dir, info_dir


def pack(pack_dir: Path, info_dir: Path):
    WHL_TAGS = {
        "win-x86_64": "win_amd64",
        "win-aarch64": "win_arm64",
        "macos-x86_64": "macosx_13_0_x86_64",
        "macos-aarch64": "macosx_13_0_arm64",
        "linux-x86_64": "manylinux2014_x86_64",
        "linux-aarch64": "manylinux2014_aarch64",
        # "android-x86_64": "android_x86_64",
        # "android-aarch64": "android_arm64_v8a",
    }

    WHL_DIR.mkdir(exist_ok=True)
    pack_bin_path = pack_dir / "maa/bin"

    def edit_whl_tag(whl_tag: str):
        print(f"    Editing wheel tag to: {whl_tag}...", end="")

        wheel_path = info_dir / "WHEEL"

        lines = []
        for line in wheel_path.open():
            if line.startswith("Tag:"):
                line = f"Tag: py3-none-{whl_tag}\n"
            lines.append(line)
        with wheel_path.open("w") as f:
            f.writelines(lines)

        print("done")

    def copy_bin(platform: str):
        print(f"    Copying bin for: {platform}...", end="")

        bin_path = Path("assets") / f"MAA-{platform}" / "bin"

        shutil.copytree(bin_path, pack_bin_path, ignore=shutil.ignore_patterns("MaaPiCli*", "MaaNode*"), dirs_exist_ok=True)

        print("done")

    def clean_bin():
        print("    Cleaning bin...", end="")
        shutil.rmtree(pack_bin_path)
        print("done")

    def pack_whl(whl_tag: str):
        print(f"    Packing whl for: {whl_tag}...", end="")

        pack_path = Path("../dist") / pack_dir.name
        subprocess.run(
            [sys.executable, "-m", "wheel", "pack", pack_path],
            cwd=WHL_DIR,
            check=True,
        )

        print("done")

    for platform, whl_tag in WHL_TAGS.items():
        print(f"Packing for: {platform}...")

        edit_whl_tag(whl_tag)
        copy_bin(platform)
        pack_whl(whl_tag)
        clean_bin()

        print("done")


def main():
    copy_base()
    set_toml_ver()
    gen_base_whl()
    paths = unpack_base_whl()
    pack(*paths)


if __name__ == "__main__":
    main()
