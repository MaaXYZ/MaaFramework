"""Run native OCR engine contracts with test/wechat_ocr/fake_sdk.cpp.

Usage: python wechat_ocr_contract_test.py <binding_dir> <binary_dir> <fake_sdk>
Compile the fixture as a shared library; it contains no proprietary runtime.
"""

import os
from pathlib import Path
import sys
import tempfile

import numpy


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    sys.path.insert(0, str(Path(sys.argv[1]).resolve()))
    os.environ["MAAFW_BINARY_PATH"] = str(Path(sys.argv[2]).resolve())
    sdk = str(Path(sys.argv[3]).resolve())
    from maa.library import Library
    from maa.pipeline import JOCR, JRecognitionType
    from maa.resource import Resource
    from maa.tasker import Tasker

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
    print("PASS: SDK ABI rejection, confidence thresholds, ROI coordinates, subpixel bounds and runtime teardown")


if __name__ == "__main__":
    main()
