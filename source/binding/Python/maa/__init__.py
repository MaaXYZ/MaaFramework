from pathlib import Path
import sys

from .library import Library

if len(sys.argv) >= 2:
    __PATH = Path(Path(sys.argv[1]).resolve())
else:
    __PATH = Path(Path(__file__).parent, "bin")

Library.open(__PATH)
