import os
import shutil
import subprocess
import sys

import tomlkit

IGNORES = ["MaaPiCli*", "MaaNode*"]


def set_toml_ver():
    TOML_FILE = "pyproject.toml"

    version = sys.argv[1]
    if "ci" in version:
        base_ver, ci_ver = version.split("ci")
        version = base_ver + "dev" + ci_ver.split("-")[0]

    print("Setting version to:", version)

    with open(TOML_FILE, "r") as f:
        py_project = tomlkit.load(f)
        py_project["project"]["version"] = version

    with open(TOML_FILE, "w") as f:
        tomlkit.dump(py_project, f)


WHL_DIR = "wheel"
DIST_DIR = "dist"
SRC_DIR = "src"


def clean_cwd():
    print("Cleaning working dir...")

    for root, dirs, files in os.walk(os.getcwd()):
        for dir_name in dirs:
            if (
                dir_name
                in (
                    SRC_DIR,
                    DIST_DIR,
                    WHL_DIR,
                    "__pycache__",
                    ".cache",
                    "build",
                )
                or ".egg-info" in dir_name
            ):

                shutil.rmtree(os.path.join(root, dir_name))
                print("    Removing", dir_name)
        for file_name in files:
            if file_name.endswith(
                (".pyc", ".pyo", ".whl", ".md"),
            ):
                os.remove(os.path.join(root, file_name))
                print("    Removing", file_name)

    print("done")


def gen_base_whl():
    print("Generating base wheel...", end="")
    subprocess.check_output(
        [sys.executable, "-m", "build", "-w"],
    )
    print("done")


def unpack_base_whl() -> str:
    """Return: unpack path"""

    print("Unpacking base wheel...", end="")

    whl_name = os.listdir(DIST_DIR)[0]
    subprocess.check_output(
        [sys.executable, "-m", "wheel", "unpack", whl_name],
        cwd=DIST_DIR,
    )

    for file in os.listdir(DIST_DIR):
        unpack_path = os.path.join(DIST_DIR, file)
        if os.path.isdir(unpack_path):
            unpack_dir = file
            break

    for file in os.listdir(unpack_path):
        if ".dist-info" in file:
            info_dir = file
            break

    info_path = os.path.join(unpack_path, info_dir)
    os.remove(os.path.join(info_path, "RECORD"))

    print("done")
    return unpack_dir, info_path


ASSETS_PATH = os.path.join("..", "..", "assets")


def pack(pack_dir: str, info_path: str):
    WHL_TAGS = {
        "win-x86_64": "win_amd64",
        # "win-aarch64": "win_arm64",
        "macos-x86_64": "macosx_13_0_x86_64",
        "macos-aarch64": "macosx_13_0_arm64",
        "linux-x86_64": "manylinux2014_x86_64",
        "linux-aarch64": "manylinux2014_aarch64",
        # "android-x86_64": "android_x86_64",
        # "android-aarch64": "android_arm64_v8a",
    }

    os.mkdir(WHL_DIR)

    wheel_path = os.path.join(info_path, "WHEEL")

    def edit_whl_tag(whl_tag: str):
        print(f"    Editing wheel tag to: {whl_tag}...", end="")

        lines = []
        for line in open(wheel_path, "r"):
            if line.startswith("Tag:"):
                line = "Tag: " + "py3-none-" + whl_tag
            lines.append(line.rstrip())
        with open(wheel_path, "w") as f:
            f.write("\n".join(lines))

        print("done")

    pack_bin_path = os.path.join(DIST_DIR, pack_dir, "maa", "bin")

    def copy_bin(platform: str):
        print(f"    Copying bin for: {platform}...", end="")

        bin_path = os.path.join(ASSETS_PATH, "MAA-" + platform, "bin")
        shutil.copytree(
            bin_path,
            pack_bin_path,
            ignore=shutil.ignore_patterns(*IGNORES),
        )

        print("done")

    def clean_bin():
        print("    Cleaning bin...", end="")
        shutil.rmtree(pack_bin_path)
        print("done")

    def pack_whl(whl_tag: str):
        print(f"    Packing whl for: {whl_tag}...", end="")

        pack_path = os.path.join("..", DIST_DIR, pack_dir)
        subprocess.check_output(
            [sys.executable, "-m", "wheel", "pack", pack_path],
            cwd=WHL_DIR,
        )

        print("done")

    for platform, whl_tag in WHL_TAGS.items():
        print(f"Packing for: {platform}...")

        edit_whl_tag(whl_tag)
        copy_bin(platform)
        pack_whl(whl_tag)
        clean_bin()

        print("done")


def copy_base():
    print("Copying base...", end="")

    PROJECT_PATH = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir)

    src_path = os.path.join(PROJECT_PATH, "source", "binding", "Python")
    shutil.copytree(src_path, SRC_DIR)

    readme_path = os.path.join(PROJECT_PATH, "README.md")
    shutil.copy(readme_path, "README.md")

    readme_path = os.path.join(PROJECT_PATH, "README_en.md")
    shutil.copy(readme_path, "README_en.md")

    license_path = os.path.join(PROJECT_PATH, "LICENSE.md")
    shutil.copy(license_path, "LICENSE.md")

    print("done")


def main():
    clean_cwd()
    set_toml_ver()
    copy_base()
    gen_base_whl()
    paths = unpack_base_whl()
    pack(*paths)


if __name__ == "__main__":
    main()
