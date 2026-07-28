from dataclasses import dataclass
import random
from typing import Any, Optional


@dataclass(frozen=True)
class GeneratedPipelineCase:
    name: str
    node: dict[str, Any]
    expected_param: Optional[dict[str, Any]]
    valid: bool


def _random_name(rng: random.Random, prefix: str) -> str:
    return f"{prefix}_{rng.randrange(100_000, 1_000_000)}"


def _random_threshold(rng: random.Random) -> float:
    return round(rng.uniform(0.1, 0.9), 4)


def _random_nms_threshold(rng: random.Random) -> float:
    return round(rng.uniform(0.2, 0.8), 4)


def _random_roi(rng: random.Random) -> list[int]:
    return [
        rng.randrange(0, 200),
        rng.randrange(0, 120),
        rng.randrange(32, 320),
        rng.randrange(32, 240),
    ]


def _as_list(value: Any) -> list[Any]:
    if isinstance(value, list):
        return list(value)
    return [value]


def _normalized_param(node: dict[str, Any]) -> dict[str, Any]:
    recognition = node["recognition"]
    param = recognition["param"] if isinstance(recognition, dict) else node

    labels = _as_list(param["labels"]) if "labels" in param else []
    expected = _as_list(param["expected"]) if "expected" in param else []
    threshold = _as_list(param["threshold"]) if "threshold" in param else [0.3]
    if not threshold:
        threshold = [0.3]
    if expected and len(threshold) == 1:
        threshold *= len(expected)

    result = {
        "roi": param.get("roi", [0, 0, 0, 0]),
        "roi_offset": param.get("roi_offset", [0, 0, 0, 0]),
        "labels": labels,
        "model": param["model"],
        "expected": expected,
        "threshold": threshold,
        "order_by": param.get("order_by", "Horizontal"),
        "index": param.get("index", 0),
    }
    for optional in ("nms", "nms_threshold"):
        if optional in param:
            result[optional] = param[optional]
    return result


def _case(
    name: str,
    param: dict[str, Any],
    *,
    v2: bool,
    valid: bool = True,
) -> GeneratedPipelineCase:
    if v2:
        node = {
            "recognition": {
                "type": "NeuralNetworkDetect",
                "param": param,
            },
        }
    else:
        node = {
            "recognition": "NeuralNetworkDetect",
            **param,
        }
    return GeneratedPipelineCase(
        name=name,
        node=node,
        expected_param=_normalized_param(node) if valid else None,
        valid=valid,
    )


def generate_cases(seed: int, count: int) -> list[GeneratedPipelineCase]:
    rng = random.Random(seed)

    flat_full_threshold = _random_threshold(rng)
    flat_full_expected = [0, 2]
    v2_array_expected = [1, 3]
    v2_roi_expected = [0, 2, 3]

    cases = [
        _case(
            "GeneratedNNDetectFlatMinimal",
            {"model": _random_name(rng, "flat_minimal") + ".onnx"},
            v2=False,
        ),
        _case(
            "GeneratedNNDetectV2Minimal",
            {"model": _random_name(rng, "v2_minimal") + ".onnx"},
            v2=True,
        ),
        _case(
            "GeneratedNNDetectFlatFull",
            {
                "model": _random_name(rng, "flat_full") + ".onnx",
                "labels": [_random_name(rng, "label") for _ in range(3)],
                "expected": flat_full_expected,
                "threshold": flat_full_threshold,
                "roi": _random_roi(rng),
                "roi_offset": [rng.randrange(-8, 9) for _ in range(4)],
                "nms": "None",
                "nms_threshold": _random_nms_threshold(rng),
                "order_by": "Score",
                "index": -rng.randrange(1, 5),
            },
            v2=False,
        ),
        _case(
            "GeneratedNNDetectV2Array",
            {
                "model": _random_name(rng, "v2_array") + ".onnx",
                "labels": [_random_name(rng, "label") for _ in range(4)],
                "expected": v2_array_expected,
                "threshold": [
                    _random_threshold(rng) for _ in v2_array_expected
                ],
                "nms": "ClassAwareIoU",
                "nms_threshold": _random_nms_threshold(rng),
                "order_by": "Area",
                "index": rng.randrange(0, 4),
            },
            v2=True,
        ),
        _case(
            "GeneratedNNDetectFlatScalar",
            {
                "model": _random_name(rng, "flat_scalar") + ".onnx",
                "labels": _random_name(rng, "label"),
                "expected": 0,
                "threshold": _random_threshold(rng),
                "nms": "CandidateCoverage",
                "nms_threshold": _random_nms_threshold(rng),
                "order_by": "Expected",
            },
            v2=False,
        ),
        _case(
            "GeneratedNNDetectV2Roi",
            {
                "model": _random_name(rng, "v2_roi") + ".onnx",
                "labels": [_random_name(rng, "label") for _ in range(4)],
                "expected": v2_roi_expected,
                "threshold": [_random_threshold(rng) for _ in v2_roi_expected],
                "roi": _random_roi(rng),
                "nms_threshold": _random_nms_threshold(rng),
                "order_by": rng.choice(
                    [
                        "Horizontal",
                        "Vertical",
                        "Score",
                        "Area",
                        "Random",
                        "Expected",
                    ]
                ),
            },
            v2=True,
        ),
        _case(
            "GeneratedNNDetectInvalidNms",
            {
                "model": _random_name(rng, "invalid_nms") + ".onnx",
                "nms": _random_name(rng, "InvalidNms"),
            },
            v2=False,
            valid=False,
        ),
        _case(
            "GeneratedNNDetectInvalidThresholdCount",
            {
                "model": _random_name(rng, "invalid_threshold") + ".onnx",
                "labels": [_random_name(rng, "label") for _ in range(4)],
                "expected": [0, 1, 3],
                "threshold": [_random_threshold(rng) for _ in range(2)],
            },
            v2=True,
            valid=False,
        ),
    ]

    if count != len(cases):
        raise ValueError(f"expected count={len(cases)}, got {count}")
    return cases
