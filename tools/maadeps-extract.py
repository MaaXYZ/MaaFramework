#!/usr/bin/env python3
import sys
from pathlib import Path
import shutil

basedir = Path(__file__).parent.parent
maadeps_dir = Path(basedir, "3rdparty", "MaaDeps")
archive_dir = Path(maadeps_dir, "tarball")

def detect_host_triplet():
    import platform
    machine = platform.machine().lower()
    system = platform.system().lower()
    if machine in {"amd64", "x86_64"}:
        machine = "x64"
    elif machine in {"x86", "i386", "i486", "i586", "i686"}:
        machine = "x86"
    elif machine in {"armv7l", "armv7a", "arm", "arm32"}:
        machine = "arm"
    elif machine in {"arm64", "armv8l", "aarch64"}:
        machine = "arm64"
    else:
        raise Exception("unsupported architecture: " + machine)
    if system in {"windows", "linux"}:
        pass
    elif 'mingw' in system or 'cygwin' in system:
        system = "windows"
    elif system == "darwin":
        system = "osx"
    else:
        raise Exception("unsupported system: " + system)
    return f"{machine}-{system}"


def main():
    if len(sys.argv) == 2:
        target_triplet = sys.argv[1]
    else:
        target_triplet = detect_host_triplet()

    devel_name = f"MaaDeps-{target_triplet}-devel.tar.xz"
    runtime_name = f"MaaDeps-{target_triplet}-runtime.tar.xz"

    devel = archive_dir / devel_name
    runtime = archive_dir / runtime_name

    if not devel.exists() or not runtime.exists():
        print(f"""Please download {devel_name} and {runtime_name} from https://github.com/MaaXYZ/MaaDeps/releases/latest, and put them into {archive_dir}""")
        archive_dir.mkdir(parents=True, exist_ok=True)
        return

    print("extracting", devel)
    shutil.unpack_archive(devel, maadeps_dir)

    print("extracting", runtime)
    shutil.unpack_archive(runtime, maadeps_dir)

if __name__ == "__main__":
    main()
