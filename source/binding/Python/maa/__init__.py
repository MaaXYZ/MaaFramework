from pathlib import Path
import sys

from .library import Library

__PATH = Path(Path(__file__).parent, "bin")
if not __PATH.exists():
    # sys.argv is designed for Github Action,we seldom use this.
    if len(sys.argv) < 2:
        raise FileNotFoundError(f"`{__PATH}` is not existed.")

    __PATH = Path(Path(sys.argv[1]).resolve(), "bin")
    if not __PATH.exists():
        raise FileNotFoundError(f"`{__PATH}` is not existed.")

ver = Library.open(__PATH)
if not ver:
    raise RuntimeError("Fail to open the library.")
