import cv2
import os

print("Usage:\n"  
      "Put the 16:9 images under ./src, and run this script, it will be auto converted to 720p.\n"
      "Drag mouse to select ROI, press 'S' to save, press 'Q' to quit.\n"
      "The cropped images will be saved in ./dst\n")

# 初始化参考点列表和布尔值标志:是否正在执行裁剪  
refPt = []
cropping = False

# 点击并裁剪ROI区域
def click_and_crop(event, x, y, flags, param):
    global refPt, cropping

    if event == cv2.EVENT_LBUTTONDOWN:
        refPt = [(x, y)]
        cropping = True

    elif event == cv2.EVENT_LBUTTONUP:
        refPt.append((x, y))
        cropping = False

        draw = image.copy()
        cv2.rectangle(draw, refPt[0], refPt[1], (0, 255, 0), 2)
        cv2.imshow("image", draw)
        
std_width = 720
std_height = 1280
std_ratio = std_width / std_height

cv2.namedWindow("image")
cv2.setMouseCallback("image", click_and_crop)  

for filename in os.listdir("./src"):
    if not filename.endswith(".png"):
        continue

    print("src:", filename)
    image = cv2.imread("./src/" + filename)

    cur_ratio = image.shape[1] / image.shape[0]

    if cur_ratio <= std_ratio:
        dsize_width = (int)(std_height * cur_ratio)
        dsize_height = std_height
    else:
        dsize_width = std_width  
        dsize_height = std_width / cur_ratio

    dsize = (dsize_width, dsize_height)
    image = cv2.resize(image, dsize, interpolation=cv2.INTER_AREA)

    while True:
        cv2.imshow("image", image)
        key = cv2.waitKey(0) & 0xFF
        if key == ord("s"):
            break
        elif key == ord("q"):
            exit()
            
    if len(refPt) == 2:
        if refPt[0][0] > refPt[1][0] or refPt[0][1] > refPt[1][1]:
            refPt[0], refPt[1] = refPt[1], refPt[0]
            
        left = refPt[0][0]
        right = refPt[1][0] 
        top = refPt[0][1]
        bottom = refPt[1][1]

        roi = image[top:bottom, left:right]
        
        horizontal_expansion = 100
        vertical_expansion = 100

        filename_x = (int)(left - horizontal_expansion / 2)
        if filename_x < 0:
            filename_x = 0
        filename_y = (int)(top - vertical_expansion / 2)
        if filename_y < 0:
            filename_y = 0
        filename_w = (right - left) + horizontal_expansion
        if filename_x + filename_w > dsize_width:
            filename_w = dsize_width - filename_x
        filename_h = (bottom - top) + vertical_expansion
        if filename_y + filename_h > dsize_height:
            filename_h = dsize_height - filename_y

        dst_filename = f'{filename}_{filename_x},{filename_y},{filename_w},{filename_h}.png'
        print('dst:', dst_filename)

        print(f"original roi: {left}, {top}, {right - left}, {bottom - top}, \n"
              f"amplified roi: {filename_x}, {filename_y}, {filename_w}, {filename_h}\n\n")

        cv2.imwrite('./dst/' + dst_filename, roi)

    refPt = []
    cropping = False
        
cv2.destroyAllWindows()