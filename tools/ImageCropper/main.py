import cv2
import os
import sys
import numpy as np
import colormatcher
from adbutils import adb
from enum import Enum
from datetime import datetime
from roimage import Roimage

class MaaCtrlOptionEnum(Enum):
    MaaCtrlOption_ScreenshotTargetLongSide = 1
    MaaCtrlOption_ScreenshotTargetShortSide = 2

# 截图参数
MaaCtrlOption: MaaCtrlOptionEnum = MaaCtrlOptionEnum.MaaCtrlOption_ScreenshotTargetShortSide
MaaCtrlOptionValue: int = 720

# 初始窗口大小 (width, height)
# 横屏
window_size = (1280, 720)
# 竖屏
# window_size = (720, 1280)

# 初始化设备参数
device = None
device_serial = None
# device_serial = "127.0.0.1:16384"

# ROI 放大方法
def amplify(rect: list[int]) -> list[int]:
    x, y, w, h = rect
    return [x - 50, y - 50, w + 100, h + 100]

# 颜色匹配方法
def matchColor(image) -> tuple[int, list[tuple[list[int]]]]:
    # framework ColorMatch common method:
    #   4.COLOR_BGR2RGB 40.COLOR_BGR2HSV 6.COLOR_BGR2GRAY
    # colormatcher method:
    #   1.Simple 2.RGBDistance
    method = cv2.COLOR_BGR2RGB
    reverse = cv2.COLOR_RGB2BGR # 不需要逆转时为None，用于显示 'MainColors' 窗口
    cluster_colors = colormatcher.kmeansClusterColors(image, method)
    return method, reverse, colormatcher.RGBDistance(cluster_colors)

# -----------------------------------------------

print("Usage: python3 main.py [device serial]\n"
     f"Current target size is based on {MaaCtrlOption}: {MaaCtrlOptionValue}.\n"
      "Put the images under ./src, and run this script, it will be auto converted to target size.\n"
      "Hold down the left mouse button, drag mouse to select a ROI.\n"
      "Hold down the right mouse button, drag mouse to move the image.\n"
      "Use the mouse wheel to zoom the image.\n"
      "press 'S' or 'ENTER' to save ROIs.\n"
      "press 'F' to save a full standardized screenshot.\n"
      "press 'R' to output only the ROI ranges, not save.\n"
      "press 'c' or 'C' (with connected) to output the ROI ranges and colors, not save.\n"
      "press 'Z' or 'DELETE' or 'BACKSPACE' to remove the latest ROI.\n"
      "press '0' ~ '9' to resize the window.\n"
      "press 'Q' or 'ESC' to quit.\n"
      "The cropped images will be saved in ./dst.\n")

# 解析命令行参数
if len(sys.argv) > 1:
    device_serial = sys.argv[1]
    print(adb.connect(device_serial))
if device_serial is not None:
    device = adb.device(device_serial)
else:
    device_list = adb.device_list()
    if len(device_list):
        for i, d in enumerate(device_list):
            print(i, '|', d)
        i = input("Please select the device (ENTER to pass): ")
        if len(i) == 1 and 0 <= int(i) < len(device_list):
            device = device_list[int(i)]

# 初始化 Roi
std_roimage: Roimage = Roimage(window_size[0], window_size[1]) # 标准化截图
win_roimage: Roimage = Roimage(0, 0, 0, 0, std_roimage)       # 相对 std_roimage ，窗口显示的区域
crop_list: list[Roimage] = []                                 # 相对 std_roimage ，需要裁剪的区域

# 初始化参数
win_name = "image"  # 窗口名
trackbars_name = "trackbars" # 轨迹条窗口名
file_name = "image" # 文件名
files = [f for f in os.listdir("./src") if f.endswith('.png')]

# -----------------------------------------------

# OpenCV 鼠标回调
# -events 鼠标事件（如按下鼠标左键，释放鼠标左键，鼠标移动等）
# -x x坐标
# -y y坐标
# -flages params 其他参数
def mouse(event, x, y, flags, param) -> None:
    global crop_end
    crop_end = Roimage(0, 0, x, y, win_roimage).getRoiInRoot()
    crop(event, x, y, flags, param)
    zoom(event, x, y, flags, param)
    move(event, x, y, flags, param)
    show_roi(crop_end)

# 显示 Roi
trackbars_img = np.ones((100, 400, 3), dtype=np.uint8) * 255
def show_roi(roi: Roimage):
    trackbars_img.fill(255)
    cv2.putText(trackbars_img, f'{roi.rectangle}', (0, 60), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 0), 2)
    cv2.imshow(trackbars_name, trackbars_img)

# 计算绘图四边形坐标
# -rectPts 相对于 std_roimage 的两点坐标 ((left,top),(right,bottom))
def count_draw_coordinate(rectPts):
    z = win_roimage.zoom
    x, y = win_roimage.point
    return ((int(rectPts[0][0]*z-x), int(rectPts[0][1]*z-y)), (int(rectPts[1][0]*z-x), int(rectPts[1][1]*z-y)))

# 绘图
# -rois 相对于 std_roimage 的 crop_roimage 列表
def draw(rois: list[Roimage]) -> None:
    img = win_roimage.image.copy()
    for roi in rois:
        pt1, pt2 = count_draw_coordinate(roi.rectanglePoints)
        cv2.rectangle(img, pt1, pt2, (0, 255, 0), 2)
    cv2.imshow(win_name, img)

# 左键裁剪ROI区域
crop_start: Roimage | None = None # 相对 win_roimage ，正在裁剪的区域
crop_end = Roimage(0, 0, 0, 0, win_roimage).getRoiInRoot()
def crop(event, x, y, flags, param) -> None:
    global crop_start, crop_end
    if event == cv2.EVENT_LBUTTONDOWN: # 按下左键
        # 记录（x,y）坐标
        crop_start = Roimage(0, 0, x, y, win_roimage)
    elif crop_start is not None and (event == cv2.EVENT_LBUTTONUP or (event == cv2.EVENT_MOUSEMOVE and (flags & cv2.EVENT_FLAG_LBUTTON))): # 释放左键 或 按住左键拖曳
        # 修正（x,y）坐标
        w, h = win_roimage.size
        x = max(0, min(w - 1, x))
        y = max(0, min(h - 1, y))
        # 记录（x,y）坐标
        crop_end = crop_start.getCropRoi(x, y).getRoiInRoot()
        # 绘图
        rois: list[Roimage] = crop_list.copy()
        rois.append(crop_end)
        draw(rois)
        # 保存 crop Roi
        if event == cv2.EVENT_LBUTTONUP:
            crop_list.append(crop_end.copy(std_roimage))
            crop_start = None

# 计算缩放倍数
# -flag 鼠标滚轮上移或下移的标识
# -zoom 缩放倍数
# -step 缩放系数，滚轮每步缩放0.1
def count_zoom(flag, zoom: float, step: float = 0.1):
    if flag > 0:  # 滚轮上移
        zoom += step
        if zoom > 3:  # 最多只能放大到 g_image 三倍大
            zoom = 3
    else:  # 滚轮下移
        zoom -= step
        if zoom < 1:  # 最多只能缩小到 g_image 大小
            zoom = 1
    return zoom

# 滚轮放大
def zoom(event, x, y, flags, param) -> None:
    global win_roimage
    if event == cv2.EVENT_MOUSEWHEEL:  # 滚轮
        z = count_zoom(flags, win_roimage.zoom)
        wx, wy, w, h = win_roimage.rectangle
        x = int((wx + x) * z / win_roimage.zoom - x)
        y = int((wy + y) * z / win_roimage.zoom - y)
        win_roimage = Roimage(w, h, x, y, win_roimage.parent, z)
        draw(crop_list)

# 计算移动后的坐标
# -pt0 win_roimage 的原始坐标
# -pt1 鼠标按下右键时的坐标
# -pt2 鼠标当前坐标
def count_move_coordinate(pt0, pt1, pt2):
    return (pt0[0] + pt1[0] - pt2[0], pt0[1] + pt1[1] - pt2[1])

# 右键拖曳
move_start = (0, 0)
move_start_roi = (0, 0)
def move(event, x, y, flags, param) -> None:
    global move_start, move_start_roi, win_roimage
    if event == cv2.EVENT_RBUTTONDOWN:
        move_start = (x, y)
        move_start_roi = win_roimage.point
    elif event == cv2.EVENT_RBUTTONUP or (event == cv2.EVENT_MOUSEMOVE and (flags & cv2.EVENT_FLAG_RBUTTON)):
        x, y = count_move_coordinate(move_start_roi, move_start, (x, y))
        win_roimage = Roimage(win_roimage.width, win_roimage.height, x, y, std_roimage, win_roimage.zoom)
        draw(crop_list)

# 轨迹条回调
# -pos 轨迹条位置
def trackbar_change(pos) -> None:
    pos = pos / 100 # get a scaling factor from trackbar pos
    w = int(std_roimage.width * pos) # scale w
    h = int(std_roimage.height * pos) # scale h
    cv2.resizeWindow(win_name, w, h) # resize window

# 标准化图片
# -image 被标准化的图片
def getStdSize(image) -> tuple[int, int]:
    # https://github.com/MaaAssistantArknights/MaaFramework/blob/main/source/MaaFramework/Controller/ControllerAgent.cpp
    # bool ControllerMgr::check_and_calc_target_image_size(const cv::Mat& raw)
    cur_height, cur_width, _ = image.shape
    scale = cur_width / cur_height
    if MaaCtrlOption is MaaCtrlOptionEnum.MaaCtrlOption_ScreenshotTargetShortSide:
        if cur_width > cur_height:
            width = MaaCtrlOptionValue * scale
            height = MaaCtrlOptionValue
        else:
            width = MaaCtrlOptionValue
            height = MaaCtrlOptionValue / scale
    else:
        # MaaCtrlOption is MaaCtrlOptionEnum.MaaCtrlOption_ScreenshotTargetLongSide
        if cur_width > cur_height:
            width = MaaCtrlOptionValue
            height = MaaCtrlOptionValue / scale
        else:
            width = MaaCtrlOptionValue * scale
            height = MaaCtrlOptionValue
    return (int(width), int(height))

# 读取文件
# -file 文件名
def readfile(file: str):
    print("src:", f"{os.getcwd()}\src\{file}")
    return cv2.imread("./src/" + file)

# 截图
def screenshot():
    if device is None:
        return None
    print("Screenshot in progress...")
    image = np.array(device.screenshot().convert('RGB'))
    image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    print("Screenshot completed.")
    return image

# 获取标准化的 Roimage
def getStdRoimage() -> Roimage | None:
    global file_name
    if len(files):
        file_name = files.pop(0)
        image = readfile(file_name)
        file_name = file_name.split(".")[0]
    else:
        image = screenshot()
        file_name = datetime.now().strftime('%H%M%S') # '%Y%m%d%H%M%S'
    if image is None:
        return None
    width, height = getStdSize(image)
    roimage = Roimage(width, height)
    roimage.image = cv2.resize(image, roimage.size, interpolation=cv2.INTER_AREA)
    return roimage

# 获取放大后的 Roi 四边形
# -roi: 需要放大的 Roi
def getAmplifiedRoiRectangle(roi: Roimage) -> list[int]:
    x, y, w, h = amplify(roi.rectangle)
    return Roimage(w, h, x, y, roi.parent).rectangle

# 初始化 cv2 窗口
cv2.namedWindow(win_name, cv2.WINDOW_NORMAL)
cv2.setMouseCallback(win_name, mouse)
cv2.namedWindow(trackbars_name, cv2.WINDOW_NORMAL)
cv2.setWindowProperty(trackbars_name, cv2.WND_PROP_TOPMOST, 1)
cv2.createTrackbar('Scale', trackbars_name, 100, 200, trackbar_change)

cropping = False
while True:
    if not cropping:
        std_roimage = getStdRoimage()
        if std_roimage is None:
            break
        win_roimage = Roimage(0, 0, 0, 0, std_roimage)
        crop_list.clear()
    draw(crop_list)

    key = cv2.waitKey(0) & 0xFF
    cropping = True
# q Q esc
    if key in [ord("q"), ord("Q"), 27]:
        cv2.destroyAllWindows()
        exit()
# 0
    if key == 48:
        cv2.setTrackbarPos('Scale', trackbars_name, 100)
        continue
# 1 ~ 5
    if key in range(49,54):
        cv2.setTrackbarPos('Scale', trackbars_name, 100 + 15 * (key - 48))
        continue
# 6 ~ 9
    if key in range(54,58):
        cv2.setTrackbarPos('Scale', trackbars_name, 100 - 15 * (58 - key))
        continue
# z Z delete backspace
    if key in [ord("z"), ord("Z"), 0, 8]:
        if len(crop_list):
            crop_list.pop()
        continue

    cropping = False
    needSave = True
    needColorMatch = False
    mains = []
# r R
    if key in [ord("r"), ord("R")]:
        needSave = False
# c C
    elif key in [ord("c")]:
        needSave = False
        needColorMatch = True
        connected = False
    elif key in [ord("C")]:
        needSave = False
        needColorMatch = True
        connected = True
# f F
    elif key in [ord("f"), ord("F")]:
        crop_list.append(Roimage(0, 0, 0, 0, std_roimage))
# s S enter
    elif key not in [ord("s"), ord("S"), ord("\r"), ord("\n")]:
        continue

    for roi in crop_list:
        print("")
        img = roi.image

        if needSave:
            x1,y1,w1,h1 = roi.rectangle
            x2,y2,w2,h2 = getAmplifiedRoiRectangle(roi)
            dst_filename: str = f'{file_name}_{x1}_{y1}_{w1}_{h1}__{x2}_{y2}_{w2}_{h2}.png'
            print(f"dst: {os.getcwd()}\dst\{dst_filename}")
            cv2.imwrite('./dst/' + dst_filename, roi.image)

        print(f"original roi: {roi.rectangle}\n"
              f"amplified roi: {getAmplifiedRoiRectangle(roi)}")

        if needColorMatch:
            method, reverse, colors = matchColor(img)
            ret = { "recognition": "ColorMatch", "roi": [], "method": method, "lower": [], "upper": [], "count": [], "connected": connected }
            mainColors = []
            for center, lower, upper in colors:
                count = colormatcher.getCount(img, lower, upper, connected, method)
                ret["lower"].append(lower)
                ret["upper"].append(upper)
                ret["count"].append(count)
                mainColor = np.zeros((80, 200, 3), dtype=np.uint8)
                mainColor[:28, :] = lower
                mainColor[28: 58, :] = upper
                mainColor[58:, :] = center
                if reverse is not None:
                    mainColor = cv2.cvtColor(mainColor, reverse)
                cv2.putText(mainColor, f'{lower}', (0, 20), cv2.FONT_HERSHEY_SIMPLEX, .75, (0, 0, 0), 2)
                cv2.putText(mainColor, f'{upper}', (0, 50), cv2.FONT_HERSHEY_SIMPLEX, .75, (255, 255, 255), 2)
                cv2.putText(mainColor, f'{count}', (66, 76), cv2.FONT_HERSHEY_SIMPLEX, .75, (127, 127, 127), 2)
                mainColors.append(mainColor)
            mains.append(cv2.hconcat(mainColors))
            cv2.namedWindow('MainColors', cv2.WINDOW_NORMAL)
            cv2.imshow('MainColors', cv2.vconcat(mains))
            print(f"ColorMatch: {ret}"
                  .replace("'", '"')
                  .replace("False", "false")
                  .replace("True", "true"))
        elif cv2.getWindowProperty('MainColors',cv2.WND_PROP_VISIBLE) > 0:
            cv2.destroyWindow('MainColors')

        print("")

print("Press any key to exit...")
cv2.waitKey(0)
cv2.destroyAllWindows()
