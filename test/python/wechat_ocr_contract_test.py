"""Run native OCR engine contracts with test/wechat_ocr/fake_sdk.cpp.

Usage: python wechat_ocr_contract_test.py <binding_dir> <binary_dir> <fake_sdk>
Compile the fixture as a shared library; it contains no proprietary runtime.
"""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

import numpy


def shared_library_filename(stem: str) -> str:
    return f"{stem}.dll" if sys.platform == "win32" else f"lib{stem}.so"


def discovery_contracts(binding_dir: Path, binary_dir: Path, sdk: Path):
    framework_name = shared_library_filename("MaaFramework")
    sdk_name = shared_library_filename("WeChatOCR")
    for placement in ("bin", "lib", "cwd"):
        with tempfile.TemporaryDirectory() as temporary:
            deployment = Path(temporary)
            for directory in ("bin", "lib", "cwd"):
                (deployment / directory).mkdir()
            shutil.copy2(binary_dir / framework_name, deployment / "bin" / framework_name)
            shutil.copy2(sdk, deployment / placement / sdk_name)
            environment = os.environ.copy()
            environment["MAA_WECHAT_TEST_DEPENDENCIES"] = str(binary_dir)
            environment["PATH"] = os.pathsep.join((str(deployment / "cwd"), str(binary_dir), environment.get("PATH", "")))
            environment["LD_LIBRARY_PATH"] = os.pathsep.join((str(binary_dir), environment.get("LD_LIBRARY_PATH", "")))
            subprocess.run(
                [sys.executable, str(Path(__file__).resolve()), str(binding_dir), str(deployment / "bin"), str(sdk),
                 "--discovery", "missing" if placement == "cwd" else "found"],
                cwd=deployment / "cwd", env=environment, check=True, timeout=30,
            )


def default_configuration(mode: str, sdk: str):
    from maa.library import Library
    from maa.pipeline import JOCR, JRecognitionType
    from maa.resource import Resource
    from maa.tasker import Tasker

    resource = Resource()
    if mode == "missing":
        assert not resource.configure_wechat_ocr(), "SDK discovery must not search cwd or PATH"
        return
    assert resource.configure_wechat_ocr(), "default SDK and runtime discovery must succeed"
    tasker = Tasker()
    assert Library.framework().MaaTaskerBindResource(tasker._handle, resource._handle)
    detail = tasker.post_recognition(
        JRecognitionType.OCR, JOCR(engine="WeChatOCR", threshold=0, expected=["开始"]),
        numpy.zeros((10, 20, 3), dtype=numpy.uint8),
    ).wait().get()
    assert detail and detail.nodes[0].recognition.hit
    assert detail.nodes[0].recognition.raw_detail["all"][0]["text"] == "开始|auto|scan_system=1|roots=0"
    del tasker
    assert resource.clear()
    assert resource.configure_wechat_ocr({}), "an explicit empty config must also discover"
    assert resource.clear()
    assert not resource.configure_wechat_ocr({"sdk_path": str(Path(sdk).parent / "missing-sdk")}), "explicit paths take priority"
    assert not resource.configure_wechat_ocr({"runtime_dir": str(Path(sdk).parent)}), "partial runtime paths must be rejected"
    assert not resource.configure_wechat_ocr({"ocr_path": sdk}), "partial runtime paths must be rejected"
    assert resource.configure_wechat_ocr({"sdk_path": sdk}), "explicit SDK paths may still use runtime discovery"
    assert resource.clear()


def recognized_text(resource) -> str:
    from maa.library import Library
    from maa.pipeline import JOCR, JRecognitionType
    from maa.tasker import Tasker

    tasker = Tasker()
    assert Library.framework().MaaTaskerBindResource(tasker._handle, resource._handle)
    detail = tasker.post_recognition(
        JRecognitionType.OCR,
        JOCR(engine="WeChatOCR", threshold=0, expected=["开始"]),
        numpy.zeros((10, 20, 3), dtype=numpy.uint8),
    ).wait().get()
    assert detail and detail.nodes[0].recognition.hit
    text = detail.nodes[0].recognition.raw_detail["all"][0]["text"]
    del tasker
    return text


def toolkit_contracts(sdk: Path):
    from maa.resource import Resource

    legacy_sdk = sdk.with_name(shared_library_filename("WeChatOCRLegacyTestSDK"))
    assert legacy_sdk.is_file(), "build and install both WeChatOCR test SDK fixtures"
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        search_roots = [root / "便携安装", root / "second-root"]
        for path in search_roots:
            path.mkdir()
        paths = [str(path) for path in search_roots]
        for scan_system in (True, False):
            resource = Resource()
            config = {"sdk_path": str(sdk), "search_roots": paths.copy(), "scan_system": scan_system}
            assert resource.configure_wechat_ocr(config)
            config["search_roots"].clear()
            expected = f"开始|auto|scan_system={int(scan_system)}|roots=2|0:{paths[0]}|1:{paths[1]}"
            assert recognized_text(resource) == expected, "forward and preserve every UTF-8 search root and discovery flag"
            assert resource.clear()

        resource = Resource()
        assert resource.configure_wechat_ocr({"sdk_path": str(sdk), "search_roots": []})
        assert recognized_text(resource) == "开始|auto|scan_system=1|roots=0", "system discovery is enabled by default"
        assert resource.clear()
        for invalid in (
            {"scan_system": False},
            {"scan_system": False, "search_roots": []},
            {"scan_system": 1},
            {"search_roots": "not-an-array"},
            {"search_roots": [1]},
            {"search_roots": [""]},
            {"search_roots": ["relative-root"]},
            {"search_roots": [str(root) + "\x00suffix"]},
            {"search_roots": [str(root)] * 65},
            {"runtime_dir": str(root), "search_roots": paths},
            {"ocr_path": str(root / "controlled-runtime"), "search_roots": paths},
        ):
            assert not resource.configure_wechat_ocr({"sdk_path": str(sdk), **invalid}), invalid

        entry = root / "controlled-runtime"
        entry.touch()
        explicit = {"sdk_path": str(legacy_sdk), "runtime_dir": str(root), "ocr_path": str(entry)}
        assert resource.configure_wechat_ocr({**explicit, "scan_system": False})
        assert recognized_text(resource) == "开始", "explicit runtimes retain the SDK 0.1 create path"
        assert resource.clear()
        assert not resource.configure_wechat_ocr({"sdk_path": str(legacy_sdk)}), "automatic discovery requires the SDK 0.2 symbol"
        assert not resource.configure_wechat_ocr({"sdk_path": str(legacy_sdk), "search_roots": paths}), "custom roots require the SDK 0.2 symbol"
        assert not resource.configure_wechat_ocr({**explicit, "search_roots": paths}), "custom roots must not silently use the legacy create path"


def main():
    if len(sys.argv) not in (4, 6):
        raise SystemExit(__doc__)
    sys.path.insert(0, str(Path(sys.argv[1]).resolve()))
    os.environ["MAAFW_BINARY_PATH"] = str(Path(sys.argv[2]).resolve())
    sdk = str(Path(sys.argv[3]).resolve())
    dll_directory = None
    if sys.platform == "win32" and os.environ.get("MAA_WECHAT_TEST_DEPENDENCIES"):
        dll_directory = os.add_dll_directory(os.environ["MAA_WECHAT_TEST_DEPENDENCIES"])
    from maa.library import Library
    from maa.pipeline import JOCR, JRecognitionType
    from maa.resource import Resource
    from maa.tasker import Tasker

    if len(sys.argv) == 6:
        assert sys.argv[4] == "--discovery"
        default_configuration(sys.argv[5], sdk)
        return

    image = numpy.zeros((30, 40, 3), dtype=numpy.uint8)
    roi = [3, 5, 20, 10]
    with tempfile.TemporaryDirectory() as temporary:
        entry = Path(temporary) / "controlled-runtime"
        entry.touch()
        for protocol, threshold, should_hit in [(0, 0.3, False), (0, 0, True), (3, 0.3, False), (3, 0.1, True), (4, 0.8, True)]:
            resource = Resource()
            assert resource.configure_wechat_ocr(
                {"sdk_path": sdk, "runtime_dir": temporary, "ocr_path": str(entry), "protocol": protocol}
            )
            tasker = Tasker()
            assert Library.framework().MaaTaskerBindResource(tasker._handle, resource._handle)
            job = tasker.post_recognition(
                JRecognitionType.OCR,
                JOCR(engine="WeChatOCR", expected=["开始"], roi=roi, threshold=threshold),
                image,
            ).wait()
            detail = job.get()
            assert detail and len(detail.nodes) == 1
            recognition = detail.nodes[0].recognition
            assert recognition and recognition.hit == should_hit, (protocol, threshold, recognition)
            assert recognition.raw_detail["all"][0]["box"] == roi
            if protocol == 0:
                assert recognition.raw_detail["all"][0]["score"] == -1, "unknown confidence must remain explicit"
            if should_hit:
                assert list(recognition.box) == roi, "restore cropped coordinates to the full image"
            del tasker
            assert resource.clear()

        resource = Resource()
        assert not resource.configure_wechat_ocr(
            {"sdk_path": sdk, "runtime_dir": temporary, "ocr_path": str(Path(temporary) / "bad-abi")}
        ), "a mismatched SDK ABI must fail before recognition"
        assert resource.configure_wechat_ocr(
            {"sdk_path": sdk, "runtime_dir": temporary, "ocr_path": str(Path(temporary) / "bad-coordinates"), "protocol": 4}
        )
        tasker = Tasker()
        assert Library.framework().MaaTaskerBindResource(tasker._handle, resource._handle)
        detail = tasker.post_recognition(JRecognitionType.OCR, JOCR(engine="WeChatOCR", expected=["开始"]), image).wait().get()
        recognition = detail.nodes[0].recognition
        assert not recognition.hit and not recognition.all_results, "clearly invalid coordinates must fail the result"
        del tasker
        assert resource.clear()
    toolkit_contracts(Path(sdk))
    discovery_contracts(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve(), Path(sdk))
    print("PASS: SDK discovery isolation, Toolkit roots/flags, legacy compatibility, ABI, confidence, ROI, bounds and teardown")


if __name__ == "__main__":
    main()
