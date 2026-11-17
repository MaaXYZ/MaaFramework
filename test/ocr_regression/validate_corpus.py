"""Validate the source images and provenance used by the OCR regression runner."""

import argparse
import hashlib
import json
from pathlib import Path
import struct


def validate(root: Path) -> None:
    root = root.resolve()
    manifest = json.loads((root / "cases.json").read_text(encoding="utf-8"))
    if manifest.get("version") != 1 or not manifest.get("cases"):
        raise ValueError("Expected a nonempty version 1 OCR corpus")

    ids = set()
    images = set()
    for case in manifest["cases"]:
        case_id = case["id"]
        if not case_id or case_id in ids:
            raise ValueError(f"Invalid or duplicate case ID: {case_id!r}")
        ids.add(case_id)
        image = (root / case["image"]).resolve()
        if not image.is_relative_to(root):
            raise ValueError(f"{case_id}: image must be inside the corpus")
        data = image.read_bytes()
        if data[:16] != b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR":
            raise ValueError(f"{case_id}: expected an original PNG image")
        if hashlib.sha256(data).hexdigest() != case["image_sha256"]:
            raise ValueError(f"{case_id}: image SHA256 mismatch")
        if list(struct.unpack(">II", data[16:24])) != case["original_size"]:
            raise ValueError(f"{case_id}: original image dimensions mismatch")
        size = case["image_size"]
        if len(size) != 2 or any(type(value) is not int or value <= 0 for value in size):
            raise ValueError(f"{case_id}: invalid recognition image dimensions")
        expected = case["recognition"].get("expected")
        if not expected or not all(isinstance(value, str) and value for value in expected):
            raise ValueError(f"{case_id}: an expected text is required to reject empty results")
        if not case.get("source") or not case.get("source_image"):
            raise ValueError(f"{case_id}: missing issue or original attachment provenance")
        if not case.get("peers"):
            raise ValueError(f"{case_id}: at least one peer is required to exercise candidate-order comparisons")
        images.add(image)

    print(f"OCR corpus validated: {len(ids)} cases, {len(images)} original images")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("corpus", type=Path)
    args = parser.parse_args()
    validate(args.corpus)
