import os
import platform

from .library import Library

__PATH = os.path.join(os.path.dirname(__file__), "bin" if platform.system() == "Windows" else "lib")

if os.path.exists(__PATH):
    ver = Library.open(__PATH)
    if ver:
        print(f"MaaFw version: {ver}")
