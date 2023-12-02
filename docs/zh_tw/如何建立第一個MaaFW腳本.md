# 如何建立你的第一個MAA腳本?

#### 作者:Arcelibs

需要準備的相關軟體:
* 一款可開啟adb測試的安卓模擬器(推薦BlueStacks)
* MaaFramework本體(後面稱MaaFW)
* GUI或CLI啟動介面(這邊使用Maay)

---
### MaaFrameWork下載與腳本編寫

參考瑪麗佬寫的基礎文件結構，我們可以得知
```
my_resource
├── image
│   ├── my_image_1.png
│   └── my_image_2.png
├── model
│   └── ocr
│       ├── det.onnx
│       ├── keys.txt
│       └── rec.onnx
├── pipeline
│   ├── my_pipeline_1.json
│   └── my_pipeline_2.json
└── properties.json
```
因此對照資料夾的結構是
![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/db86a822-9a9f-4241-b72f-b548e25d5b3a)


image資料夾放的是截圖相關的圖片資源，以設計一項收取禮物資源的需求來說

我將禮物盒、返回鈕、一次領取等功能都截圖下來了
![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/e73fc7d7-8641-47cc-824b-957351682317)


model資料夾放的是OCR識別模型，可從[MaaCommonAssets](https://github.com/MaaAssistantArknights/MaaCommonAssets/tree/main/OCR)取得

需要注意如果是日文遊戲則使用日文模型，別弄錯了

pipeline資料夾則是腳本實際編寫的內容，可參考[這個](https://github.com/MaaAssistantArknights/MaaFramework/blob/main/docs/zh_cn/3.1-%E4%BB%BB%E5%8A%A1%E6%B5%81%E6%B0%B4%E7%BA%BF%E5%8D%8F%E8%AE%AE.md)

---
### 編寫任務DEMO

以下是我針對idolypride寫的，收取禮物盒功能的需求

主任務名稱為awards，下一個執行任務為Gift

![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/ac81a04c-ae1c-4635-8271-e90bda2fc7c8)


Gift任務，算法我使用了TemplateMatch，就是圖片辨識

辨識Gift.png這張圖片後動作為Click點擊，下一步是GetGift
![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/465cb064-bb04-4461-a4d3-48116d3859f7)


GetGift任務，這邊我加上了Roi，Roi的取得能幫助MaaFW更精準找圖

關於如何取得Roi跟完美的圖片，可參考這個工具 [圖片及 ROI 取得工具](https://github.com/MaaAssistantArknights/MaaFramework/tree/main/tools/ImageCropper)
![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/b6599d71-6d61-48d3-8c03-8ac75c3b419d)


---
### Maay的使用

首先下載Maay後解壓縮是這樣

![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/2691ce3c-cc19-4038-ad8d-7ff4bff35b09)


把你前面下載的MaaFw的Bin資料夾的內容複製貼上到maay裡的maa

這樣可以保持你的maay能使用上最新的MaaFW

進入Maay後，首先打開你的模擬器，開啟模擬器的adb功能

![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/b9307007-761b-4932-b6e1-23b5bd0180b0)


按一下刷新後選取你的設備並按下連接，這邊配置可以選擇

根據推薦，可以選adb+adb+gzip這個選項

![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/a61e36ac-eeec-42de-85a0-aa3a4dd16dd8)

連上後會是這個畫面

![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/c478e3d4-9532-41d7-81fa-d22c3f6d4c75)



導入資源的部分，如果是跟Maay同一層，可以寫/MAAIP2就好

可以按下測試看看你的腳本文件是否載入成功

![image](https://github.com/Arcelibs/MaaFramework/assets/49543451/71377313-3de7-4555-abdf-d35ccabb5a62)


