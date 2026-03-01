import os
from pathlib import Path

from .library import Library

env_path = os.environ.get("MAAFW_BINARY_PATH")
internal_mode = os.environ.get("MAAFW_INTERNAL_MODE")

if env_path:
    __PATH = Path(env_path)
else:
    __PATH = Path(Path(__file__).parent, "bin")

if not internal_mode:
    Library.open(__PATH, agent_server=False)
