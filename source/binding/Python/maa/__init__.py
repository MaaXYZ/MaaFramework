import os
from pathlib import Path

from .library import Library

env_path = Path(os.environ.get("MAAFW_BINARY_PATH"))
if env_path:
    __PATH = env_path
else:
    __PATH = Path(Path(__file__).parent, "bin")

Library.open(__PATH)
