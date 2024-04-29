import os

from .library import Library

__PATH = os.path.join(os.path.dirname(__file__), "bin")

if os.path.exists(__PATH):
    ver = Library.open(__PATH)
    if ver:
        print(f"MaaFw version: {ver}")
