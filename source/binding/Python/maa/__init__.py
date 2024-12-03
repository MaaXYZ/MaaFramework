from pathlib import Path

from .library import Library

__PATH = Path(Path(__file__).parent, "bin")

# If you want to print maafw version,please use `from maa import ver`
ver = None

if __PATH.exists():
    ver = Library.open(__PATH)
    if not ver:
        raise RuntimeError(
            "Library not initialized, please call `library.open()` first."
        )
else:
    raise FileNotFoundError(__PATH, " ", "is not exists.")
