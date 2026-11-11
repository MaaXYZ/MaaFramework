"""Opt-in integration test against a user-installed WeChat OCR runtime.

Usage: python wechat_ocr_test.py <binding_dir> <binary_dir> <sdk_path>
       <runtime_dir> <ocr_path> <image_path> [--expected TEXT] [--roi X Y W H]
The image must contain the expected text inside the optional ROI. No proprietary
runtime files are downloaded or bundled by this test.
"""

import argparse
import ctypes
import json
import os
from pathlib import Path
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("binding_dir", "binary_dir", "sdk_path", "runtime_dir", "ocr_path", "image_path"):
        parser.add_argument(name, type=Path)
    parser.add_argument("--expected", default="微信文字识别测试")
    parser.add_argument("--roi", type=int, nargs=4, default=[0, 0, 0, 0])
    parser.add_argument("--protocol", type=int, choices=(0, 3, 4), default=0)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str(args.binding_dir.resolve()))
    os.environ["MAAFW_BINARY_PATH"] = str(args.binary_dir.resolve())

    from maa.buffer import ImageBuffer
    from maa.library import Library
    from maa.pipeline import JOCR, JRecognitionType
    from maa.resource import Resource
    from maa.tasker import Tasker

    image_buffer = ImageBuffer()
    decode = Library.framework().MaaImageBufferSetEncoded
    decode.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint64]
    decode.restype = ctypes.c_uint8
    encoded = args.image_path.read_bytes()
    encoded_buffer = ctypes.create_string_buffer(encoded)
    assert decode(image_buffer._handle, encoded_buffer, len(encoded)), "input image must decode"
    image = image_buffer.get()

    resource = Resource()
    assert resource.use_cpu()
    assert resource.configure_wechat_ocr(
        {
            "sdk_path": str(args.sdk_path.resolve()),
            "runtime_dir": str(args.runtime_dir.resolve()),
            "ocr_path": str(args.ocr_path.resolve()),
            "protocol": args.protocol,
            "startup_timeout_ms": 15000,
            "request_timeout_ms": 15000,
        }
    ), "SDK/runtime creation must succeed"

    with tempfile.TemporaryDirectory() as temporary:
        pipeline = Path(temporary) / "pipeline"
        pipeline.mkdir()
        (pipeline / "wechat.json").write_text(
            json.dumps({"WeChat": {"recognition": {"type": "OCR", "param": {"engine": "WeChatOCR"}}}}),
            encoding="utf-8",
        )
        assert resource.post_bundle(temporary).wait().succeeded, "a pipeline-only bundle must load"
        assert not resource.configure_wechat_ocr({}), "configuration must be locked after loading"
        tasker = Tasker()
        assert Library.framework().MaaTaskerBindResource(tasker._handle, resource._handle)
        assert tasker.inited
        job = tasker.post_recognition(
            JRecognitionType.OCR,
            JOCR(engine="WeChatOCR", expected=[args.expected], roi=args.roi),
            image,
        ).wait()
        assert job.succeeded
        detail = job.get()
        assert detail and len(detail.nodes) == 1
        recognition = detail.nodes[0].recognition
        assert recognition and recognition.hit and recognition.algorithm == "OCR"
        assert recognition.best_result and args.expected in recognition.best_result.text
        box = recognition.box
        assert box and box.w > 0 and box.h > 0
        assert box.x >= args.roi[0] and box.y >= args.roi[1], "ROI coordinates must be restored to the full image"
        assert box.x + box.w <= image.shape[1] and box.y + box.h <= image.shape[0]
        report = {"algorithm": recognition.algorithm, "box": list(box), "detail": recognition.raw_detail}
        report_text = json.dumps(report, ensure_ascii=False, indent=2)
        if args.report:
            args.report.write_text(report_text, encoding="utf-8")
        print(report_text)
        del tasker
    assert resource.clear()
    print("PASS: native OCR engine, runtime loading, model-free bundle, expected matching, ROI coordinates and teardown")


if __name__ == "__main__":
    main()
