import os
from pathlib import Path

from .library import Library

__PATH = Path(os.environ.get("MAAFW_BINARY_PATH"))
if not __PATH:
    __PATH = Path(Path(__file__).parent, "bin")

Library.open(__PATH)
