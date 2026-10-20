# 多点取色工具（网页版）

单文件网页工具，无需安装、无外部依赖，直接用浏览器打开 `index.html` 即可使用。

用于在截图上拾取颜色并生成 `MultiColorMatch` 多点找色所需的点阵代码，支持生成 page.lua 格式的点阵文件，供 pipeline 通过 `pattern` 字段引用。

## 使用

1. 用浏览器打开 `index.html`（或部署到任意静态服务器）。
2. 打开截图，左键拾取基准点与偏移点。
3. 在代码框中生成 / 粘贴点阵代码，可用「测试找色」在图上演示命中结果。
4. 将生成的点阵追加到资源的 `pattern/*.lua` 文件中，pipeline 节点使用：

```json
{
    "recognition": "MultiColorMatch",
    "pattern": "点阵名称"
}
```
