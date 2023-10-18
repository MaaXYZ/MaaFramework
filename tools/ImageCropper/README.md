# 截图工具

## 环境

需要 `python` 环境

## 依赖

```shell
python -m pip install -r requirements.txt
```

## 使用

0. 调整脚本中的 `标准化参数` `ROI 放大方法` `颜色匹配方法`
1. 如果有截图，需预先保存到 `./src/` 路径下
2. 运行 `start.bat` 或 `python main.py [device serial]` ，设备地址为可选
3. 在弹窗中左键选择目标区域，滚轮缩放图片，右键移动图片
4. 使用快捷键操作：
    - 按 <kbd>S</kbd> <kbd>ENTER</kbd> 保存目标区域
    - 按 <kbd>A</kbd> 保存标准化截图
    - 按 <kbd>R</kbd> 不保存，只输出 ROI 范围
    - 按 <kbd>C</kbd> 不保存，输出 ROI 范围和 ColorMatch 的所需字段，大写将使用 connected 字段
    - 按 <kbd>Z</kbd> <kbd>DELETE</kbd> <kbd>BACKSPACE</kbd> 撤销
    - 按 <kbd>0</kbd> ~ <kbd>9</kbd> 缩放窗口
    - 按 <kbd>Q</kbd> <kbd>ESC</kbd> 退出

5. 目标区域截图保存在 `./dst/` 路径下，文件名为 `src` 中的文件名 / 截图的时间 + ROI + 放大后的 ROI
