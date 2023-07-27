import cv2
import os
import numpy as np
from PIL import Image

print("Usage:\n"  
      "Put the images under ./src, and run this script, it will be auto converted.\n"
      "Drag mouse to select ROI, press 'S' to save, press 'Q' to quit.\n"
      "The cropped images will be saved in ./dst\n")

# 初始化参考点列表和布尔值标志:是否正在执行裁剪
refPt = []
cropping = False

# 点击并裁剪ROI区域
def click_and_crop(event, x, y, flags, param):

  # 获取全局变量的引用
  global refPt, cropping

  # 如果鼠标左被单击,记录(x,y)坐标并显示裁剪正在进行
  if event == cv2.EVENT_LBUTTONDOWN:
    refPt = [(x, y)]
    cropping = True

  # 检测鼠标左键是否释放
  elif event == cv2.EVENT_LBUTTONUP:
    # 记录结束(x,y)坐标,并显示裁剪结束
    refPt.append((x, y))
    cropping = False
    
    # 绘制矩形
    draw = image.copy()
    cv2.rectangle(draw, refPt[0], refPt[1], (0, 255, 0), 2)
    cv2.imshow("image", draw)

cv2.namedWindow("image")
cv2.setMouseCallback("image", click_and_crop)

for filename in os.listdir("./src"):
  if not filename.endswith(".png"):
    continue
  
  print("src:", filename)

  image = Image.open("./src/" + filename)
  
  # 新增轉換色彩空間
  image = image.convert('RGB')

  # 取得图像大小
  width, height = image.size

  # 比较长宽比
  ratio = width / height
  if ratio >= 1.77: 
    TARGET_WIDTH = 1280
    TARGET_HEIGHT = 720
  else:   
    TARGET_WIDTH = 720
    TARGET_HEIGHT = 1280

  #增加縮放比例(根據PC)
  scale = 0.75
  
  # 缩放图像
  # image = image.resize((TARGET_WIDTH, TARGET_HEIGHT))
  # 等比例縮放
  image = image.resize((int(TARGET_WIDTH * scale), int(TARGET_HEIGHT * scale)))

  # 转换为数组
  image = np.array(image)
  
  # 將數組轉為 BGR 格式
  image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
  
  # 正確傳入視窗名稱
  cv2.imshow("image", image)

  while True:
    cv2.imshow("image", image)
    key = cv2.waitKey(0) & 0xFF

    if key == ord("s"):
      break
    elif key == ord("q"):
      exit()

# 如果参考点列表里有俩个点,则裁剪区域并展示  
if len(refPt) == 2:

  # 调整参考点顺序,确保左上角点在前
  if refPt[0][0] > refPt[1][0] or refPt[0][1] > refPt[1][1]:
    refPt[0], refPt[1] = refPt[1], refPt[0]

  left = refPt[0][0]
  right = refPt[1][0]
  top = refPt[0][1]
  bottom = refPt[1][1]
  
  # 裁剪图片
  roi = image[top:bottom, left:right]

  # 增加裁剪范围,避免裁剪到内容边界
  horizontal_expansion = 100
  vertical_expansion = 100

  # 计算保存文件名的坐标参数
  filename_x = left - horizontal_expansion // 2
  filename_y = top - vertical_expansion // 2
  filename_w = (right - left) + horizontal_expansion
  filename_h = (bottom - top) + vertical_expansion

  # 防止坐标越界
  filename_x = max(filename_x, 0)
  filename_y = max(filename_y, 0)
  filename_w = min(filename_w, width)
  filename_h = min(filename_h, height)

  # 拼接保存文件名
  dst_filename = f'{filename}_{filename_x},{filename_y},{filename_w},{filename_h}.png'
  
  print('dst:', dst_filename)

  print(f"original roi: {left}, {top}, {right - left}, {bottom - top}")
  print(f"amplified roi: {filename_x}, {filename_y}, {filename_w}, {filename_h}\n")

  # 保存图片
  cv2.imwrite('./dst/' + dst_filename, roi)

  # 重置裁剪状态
  refPt = []
  cropping = False

# 关闭所有打开的窗口
cv2.destroyAllWindows()
