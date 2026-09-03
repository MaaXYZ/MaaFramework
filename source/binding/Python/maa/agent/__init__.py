import os
from pathlib import Path

from ..library import Library

env_path = os.environ.get("MAAFW_BINARY_PATH")
path = Path(env_path) if env_path else Path(Path(__file__).parent.parent, "bin")

Library.open(path, agent_server=True)
