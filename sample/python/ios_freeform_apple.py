#!/usr/bin/env python3
"""在真机 iPhone 的无边记里画一个苹果 logo —— iOS 控制器的端到端样例。

    python sample/python/ios_freeform_apple.py
    python sample/python/ios_freeform_apple.py --wifi 10.24.24.7

流水线（`sample/resource/pipeline/ios_freeform_apple.json`）负责"到画布里去、并把笔拿起来"：
起无边记 -> 认到看板列表就点新建 -> 认到底部工具条就点笔图标 -> 认到右上角蓝色对勾才动手画。

中间那一步不是多余的装饰：**画板默认不在绘画模式**，这时单指拖动是"平移画布"，一笔画都
不留（实测过，四条不同节奏的折线全部只挪动了点阵网格）。点了笔图标之后右上角才会冒出那个
蓝色对勾，看到它才代表可以下笔。

"画"这件事交给自定义动作：pipeline 的 `Swipe` 只能走直线，而苹果轮廓要的是一条**连续**的
笔画，抬笔重按会留下接缝。所以用 touch_down / touch_move / touch_up 描完整条。
"""
import argparse
import math
import time
from pathlib import Path

import cv2
import numpy as np

from maa.controller import IOSController
from maa.custom_action import CustomAction
from maa.resource import Resource
from maa.tasker import Tasker

SAMPLE_ROOT = Path(__file__).resolve().parents[1]

resource = Resource()

# 苹果轮廓：两条闭合笔画（带咬口的果身 + 一片斜叶子），坐标在 logo 自己的 0..1 里、y 向下。
#
# 这份点表是**量出来的**：取 simple-icons（CC0-1.0）里 Apple 图标的 SVG path，把三/二次
# 贝塞尔按 24 步展平，再用 Douglas-Peucker 抽稀到 44 + 27 个点。之前两版都是猜的——先手描
# 折点，后拿圆并/圆差凑——结果叶子斜反了（真 logo 的叶尖朝右上，我画成朝左上）、果身偏胖、
# 咬口太小。几何这种东西猜不出来，一次量准比迭代五轮便宜。
BODY = [
    (0.112, 0.290), (0.060, 0.342), (0.023, 0.409), (0.006, 0.473),
    (0.000, 0.563), (0.013, 0.655), (0.047, 0.764), (0.084, 0.840),
    (0.144, 0.926), (0.175, 0.960), (0.202, 0.982), (0.232, 0.996),
    (0.255, 1.000), (0.298, 0.994), (0.371, 0.967), (0.409, 0.960),
    (0.469, 0.963), (0.549, 0.993), (0.584, 0.998), (0.624, 0.993),
    (0.666, 0.970), (0.708, 0.927), (0.757, 0.858), (0.792, 0.793),
    (0.815, 0.733), (0.764, 0.703), (0.723, 0.661), (0.696, 0.613),
    (0.681, 0.549), (0.682, 0.501), (0.696, 0.448), (0.732, 0.389),
    (0.789, 0.341), (0.773, 0.321), (0.739, 0.289), (0.695, 0.263),
    (0.647, 0.247), (0.595, 0.241), (0.537, 0.248), (0.414, 0.287),
    (0.386, 0.282), (0.288, 0.248), (0.228, 0.245), (0.167, 0.260),
]
LEAF = [
    (0.406, 0.231), (0.420, 0.231), (0.441, 0.229), (0.461, 0.223),
    (0.481, 0.216), (0.501, 0.206), (0.519, 0.194), (0.535, 0.180),
    (0.550, 0.165), (0.563, 0.149), (0.578, 0.125), (0.596, 0.086),
    (0.602, 0.065), (0.606, 0.044), (0.607, 0.022), (0.606, 0.000),
    (0.587, 0.002), (0.567, 0.007), (0.527, 0.023), (0.490, 0.046),
    (0.474, 0.060), (0.459, 0.076), (0.443, 0.096), (0.433, 0.114),
    (0.415, 0.152), (0.409, 0.173), (0.406, 0.195),
]

# 相邻两个触摸报告之间的间隔。设备靠帧间差值算笔速，太密会被当成抖动，实测 12~20ms
# 是可用区间，取中间值。
kStepMs = 18


def _densify(pts, per_seg=2):
    """每段再插点：抽稀后的折线在 500px 画布上仍有 15~30px 的直边，不插会看出一圈多边形。"""
    out = []
    n = len(pts)
    for i in range(n):
        ax, ay = pts[i]
        bx, by = pts[(i + 1) % n]
        for k in range(per_seg):
            t = k / per_seg
            out.append((ax + (bx - ax) * t, ay + (by - ay) * t))
    out.append(pts[0])
    return out


def _strokes():
    """密化后按整体外接框归一到 0..1，保持宽高比（x、y 用同一个分母）。

    logo 是"高>宽"的，所以短的那一轴要往中间挪 (span-该轴长度)/2，否则整幅会贴着左边走。
    """
    all_pts = [_densify(p) for p in (BODY, LEAF)]
    xs = [x for pts in all_pts for x, _ in pts]
    ys = [y for pts in all_pts for _, y in pts]
    x0, y0, x1, y1 = min(xs), min(ys), max(xs), max(ys)
    span = max(x1 - x0, y1 - y0)
    off_x = (span - (x1 - x0)) / 2
    off_y = (span - (y1 - y0)) / 2
    return [[((x - x0) / span + off_x, (y - y0) / span + off_y) for x, y in pts]
            for pts in all_pts]


def _canvas(image_shape):
    """画布上能下笔的那块正方形。

    下边界停在 0.83h：绘画模式右下角那枚笔尖指示（当前笔/颜色）就在那儿，压在它上面
    会被当成点按钮，换支笔之后半张图都对不上。
    """
    height, width = image_shape[:2]
    top = int(height * 0.18)
    bottom = int(height * 0.83)
    side = min(int(width * 0.72), bottom - top)
    return (width - side) // 2, top, side


@resource.custom_action("DrawAppleLogo")
class DrawAppleLogo(CustomAction):
    def run(self, context, argv: CustomAction.RunArg) -> bool:
        controller = context.tasker.controller
        image = controller.cached_image
        if image is None or not image.size:
            print("no cached screenshot; cannot map the logo")
            return False

        left, top, side = _canvas(image.shape)
        before = image.copy()
        box = (slice(top, top + side), slice(left, left + side))

        perimeter = 0
        for stroke in _strokes():
            points = [(int(left + x * side), int(top + y * side)) for x, y in stroke]
            perimeter += sum(
                math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(points, points[1:])
            )
            if not controller.post_touch_down(*points[0]).wait():
                return False
            for x, y in points[1:]:
                if not controller.post_touch_move(x, y).wait():
                    return False
                time.sleep(kStepMs / 1000.0)
            if not controller.post_touch_up().wait():
                return False
            time.sleep(0.2)

        # 判"画出来了"不能拿整幅差分：画布自己会动（选中态、点阵网格跟着平移）。只数下笔
        # 那块矩形里变掉的像素，下限按实际笔迹长度算：周长 x 笔宽，再留一半余量。
        # 笔宽是在 720 缩放的截图上量出来的（一条直线 406px 长、变化 2436px => 6px 宽）。
        controller.post_screencap().wait()
        after = controller.cached_image
        if after is None or not after.size:
            return False
        changed = int((np.abs(after[box].astype(int) - before[box].astype(int)).sum(axis=2) > 40).sum())
        stroke_px = max(4, image.shape[1] // 120)
        expect = int(perimeter * stroke_px * 0.5)
        print(f"drawn: canvas box changed {changed} px, need >= {expect}")
        cv2.imwrite("/tmp/ios_apple_after.png", after)
        return changed >= expect


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--udid", default="", help="设备 UDID，留空用唯一在连的那台")
    ap.add_argument("--wifi", default="", help="设备的局域网地址；留空走 USB")
    ap.add_argument("--pairing-record", type=Path, help="调用方保存的配对凭据文件 / caller-owned pairing credential file")
    args = ap.parse_args()

    if not resource.post_bundle(SAMPLE_ROOT / "resource").wait():
        raise RuntimeError("failed to load sample resource")

    records = [args.pairing_record.read_text(encoding="utf-8")] if args.pairing_record else []
    controller = IOSController(udid=args.udid, wifi_address=args.wifi, pairing_records=records)
    if not controller.post_connection().wait():
        raise RuntimeError(
            "failed to connect the iPhone. Over Wi-Fi this usually means there is no "
            "explicit pairing credential; see docs/zh_cn/2.4-控制方式说明.md"
        )
    print("connected:", controller.info)

    tasker = Tasker()
    if not tasker.bind(resource, controller):
        raise RuntimeError("failed to bind resource and controller")

    job = tasker.post_task("StartFreeform")
    job.wait()
    detail = job.get()
    if detail is None:
        raise RuntimeError("task disappeared")
    # bool(detail) 只说明"拿到了一个对象"，跟成没成没关系，别看它。
    print("task succeeded:", detail.status.succeeded)
    for node in detail.nodes:
        print(f"  {node.name:14s} completed={node.completed}")


if __name__ == "__main__":
    main()
