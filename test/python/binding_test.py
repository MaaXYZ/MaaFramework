from pathlib import Path
import sys

current_dir = Path(__file__).resolve().parent

binding_dir = current_dir.parent.parent / "source" / "binding" / "Python"
binding_dir = str(binding_dir)
print(binding_dir)
if binding_dir not in sys.path:
    sys.path.insert(0, binding_dir)


from maa.library import Library
from maa.resource import Resource
from maa.controller import AdbController
from maa.instance import Instance
from maa.toolkit import Toolkit

if __name__ == "__main__":
    version = Library.open(current_dir.parent.parent / "install" / "bin")
    print(f"MaaFw Version: {version}")

    Toolkit.init_config()

    resource = Resource()
    print(f"resource: {hex(resource._handle)}")

    adb_controller = AdbController("adb", "127.0.0.1:5555")
    print(f"adb_controller: {hex(adb_controller._handle)}")

    maa_inst = Instance()
    maa_inst.bind(resource, adb_controller)
    print(f"maa_inst: {hex(maa_inst._handle)}")

    print("Done.")
