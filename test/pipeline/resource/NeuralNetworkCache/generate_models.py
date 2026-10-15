"""Regenerate the tiny, synthetic fixtures with `python -m pip install onnx`.

Both models consume float NCHW RGB input in [0, 1]. The classifier selects
class 0 for white and class 1 for black. The detector emits two disjoint
boxes for white and no boxes for black. No trained weights are needed.
"""

from pathlib import Path

from onnx import TensorProto, helper


def save(name, nodes, initializers, shape):
    graph = helper.make_graph(
        nodes,
        name,
        [helper.make_tensor_value_info("image", TensorProto.FLOAT, [1, 3, 32, 32])],
        [helper.make_tensor_value_info("output", TensorProto.FLOAT, shape)],
        initializers,
    )
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 13)])
    model.ir_version = 8
    (Path(__file__).parent / f"{name}.onnx").write_bytes(model.SerializeToString())


mean = helper.make_node("ReduceMean", ["image"], ["mean"], keepdims=0)
save(
    "classifier",
    [
        mean,
        helper.make_node("Reshape", ["mean", "shape"], ["score"]),
        helper.make_node("Sub", ["one", "score"], ["other"]),
        helper.make_node("Concat", ["score", "other"], ["output"], axis=1),
    ],
    [
        helper.make_tensor("shape", TensorProto.INT64, [2], [1, 1]),
        helper.make_tensor("one", TensorProto.FLOAT, [1, 1], [1]),
    ],
    [1, 2],
)
save(
    "detector",
    [
        mean,
        helper.make_node("Mul", ["scores", "mean"], ["confidence"]),
        helper.make_node("Concat", ["boxes", "confidence"], ["output"], axis=1),
    ],
    [
        helper.make_tensor("boxes", TensorProto.FLOAT, [1, 4, 2], [8, 24, 8, 24, 8, 8, 8, 8]),
        helper.make_tensor("scores", TensorProto.FLOAT, [1, 2, 2], [0.9, 0.1, 0.1, 0.8]),
    ],
    [1, 6, 2],
)
