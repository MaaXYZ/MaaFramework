import os
from pathlib import Path

from .library import Library

env_path = os.environ.get("MAAFW_BINARY_PATH")
if env_path:
    __PATH = Path(env_path)
else:
    __PATH = Path(Path(__file__).parent, "bin")

Library.open(__PATH, agent_server=False)
