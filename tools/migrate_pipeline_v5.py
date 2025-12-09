#!/usr/bin/env python3
"""
Pipeline JSON 迁移脚本 - 将旧版 is_sub/interrupt 转换为 v5.1 的 [JumpBack] 前缀格式

使用方法:
    python migrate_pipeline_v5.py <目录路径> [--dry-run] [--backup]

参数:
    目录路径: 包含 pipeline JSON 文件的目录
    --dry-run: 仅显示将要进行的更改，不实际修改文件
    --backup: 在修改前备份原文件（添加 .bak 后缀）

转换规则:
    1. interrupt 字段中的节点会被加上 `[JumpBack]` 前缀后合并到 next 字段
    2. 所有 is_sub: true 的节点，在被其他节点的 next/on_error 引用时会被加上 `[JumpBack]` 前缀
       （支持跨文件引用：会先扫描所有文件收集全局 is_sub 节点）
    3. 删除 is_sub 和 interrupt 字段

特性:
    - 支持 JSONC（带注释的 JSON，包括 // 和 /* */ 两种注释）
    - **保留原文件中的所有注释**
    - 保持 JSON 字段顺序不变
    - 保持原文件的缩进风格
    - 支持跨文件节点引用

示例:
    # 预览更改（不实际修改文件）
    python migrate_pipeline_v5.py ./pipeline --dry-run

    # 执行迁移并备份原文件
    python migrate_pipeline_v5.py ./pipeline --backup

    # 直接执行迁移
    python migrate_pipeline_v5.py ./pipeline
"""

import re
import os
import sys
import shutil
import argparse
from pathlib import Path
from typing import Any
from collections import OrderedDict


def remove_jsonc_comments_for_parsing(text: str) -> str:
    """
    移除 JSONC 文件中的注释用于解析，但保留原文本用于重建
    支持 // 和 /* */ 两种注释
    """
    result = []
    i = 0
    in_string = False
    string_char = None

    while i < len(text):
        if in_string:
            if text[i] == "\\" and i + 1 < len(text):
                result.append(text[i : i + 2])
                i += 2
                continue
            elif text[i] == string_char:
                in_string = False
                string_char = None
                result.append(text[i])
                i += 1
                continue
            else:
                result.append(text[i])
                i += 1
                continue

        if text[i] in ('"', "'"):
            in_string = True
            string_char = text[i]
            result.append(text[i])
            i += 1
            continue

        # 检查单行注释 //
        if text[i : i + 2] == "//":
            while i < len(text) and text[i] != "\n":
                i += 1
            continue

        # 检查多行注释 /* */
        if text[i : i + 2] == "/*":
            i += 2
            while i < len(text) and text[i : i + 2] != "*/":
                i += 1
            i += 2
            continue

        result.append(text[i])
        i += 1

    return "".join(result)


def remove_trailing_commas(text: str) -> str:
    """移除 JSON 中的尾逗号（在 ] 或 } 之前的逗号）"""
    result = []
    i = 0
    in_string = False

    while i < len(text):
        if in_string:
            if text[i] == "\\" and i + 1 < len(text):
                result.append(text[i : i + 2])
                i += 2
                continue
            elif text[i] == '"':
                in_string = False
            result.append(text[i])
            i += 1
            continue

        if text[i] == '"':
            in_string = True
            result.append(text[i])
            i += 1
            continue

        if text[i] == ",":
            j = i + 1
            while j < len(text) and text[j] in " \t\n\r":
                j += 1
            if j < len(text) and text[j] in "]}":
                i += 1
                continue

        result.append(text[i])
        i += 1

    return "".join(result)


def parse_jsonc(text: str) -> Any:
    """解析 JSONC 文件内容，返回 OrderedDict 以保持字段顺序"""
    import json

    clean_text = remove_jsonc_comments_for_parsing(text)
    clean_text = remove_trailing_commas(clean_text)
    return json.loads(clean_text, object_pairs_hook=OrderedDict)


def detect_indent(text: str) -> str:
    """检测 JSON 文件的缩进风格"""
    lines = text.split("\n")
    for line in lines:
        stripped = line.lstrip()
        if stripped and stripped[0] not in "{}[]":
            indent = line[: len(line) - len(stripped)]
            if indent:
                return indent
    return "    "  # 默认 4 空格


def format_array_value(value: list, indent: str = "    ", base_indent: str = "") -> str:
    """
    格式化数组值为多行或单行格式

    Args:
        value: 数组值
        indent: 缩进字符串
        base_indent: 当前字段的基础缩进

    Returns:
        格式化后的数组字符串（不含字段名）
    """
    import json

    if not value:
        return "[]"

    # 判断是否需要多行格式
    # 条件：超过1个元素，或单行总长度超过80字符
    json_str = json.dumps(value, ensure_ascii=False)
    should_multiline = len(value) > 1 or len(json_str) > 80

    if should_multiline:
        # 多行格式（紧凑风格）
        lines = [
            f"{base_indent}{indent}{json.dumps(item, ensure_ascii=False)}"
            for item in value
        ]
        # 为非最后一个元素添加逗号
        for i in range(len(lines) - 1):
            lines[i] += ","

        result = "[\n" + "\n".join(lines) + f"\n{base_indent}]"
        return result
    else:
        # 单行格式
        return json_str


def rebuild_json_with_comments(
    original_text: str, original_data: dict, migrated_data: dict, indent: str = "    "
) -> str:
    """
    基于原始文本和迁移后的数据重建 JSON，保留注释

    使用更精确的方式在原文本上进行替换操作
    """
    result_text = original_text

    # 遍历所有节点，应用修改
    for node_name, migrated_node_data in migrated_data.items():
        if not isinstance(migrated_node_data, dict):
            continue

        original_node_data = original_data.get(node_name, {})
        if not isinstance(original_node_data, dict):
            continue

        # 首先提取该节点的文本范围
        node_start_pattern = rf'("{re.escape(node_name)}"\s*:\s*\{{)'
        node_matches = list(re.finditer(node_start_pattern, result_text))

        if not node_matches:
            continue

        # 找到节点范围（从节点开始到节点结束}）
        for match in node_matches:
            node_start_pos = match.start()

            # 找到对应的结束括号
            brace_count = 0
            i = match.end()
            node_end_pos = -1
            in_string = False
            escape_next = False

            while i < len(result_text):
                char = result_text[i]

                if escape_next:
                    escape_next = False
                    i += 1
                    continue

                if char == "\\":
                    escape_next = True
                    i += 1
                    continue

                if char == '"' and not in_string:
                    in_string = True
                elif char == '"' and in_string:
                    in_string = False
                elif not in_string:
                    if char == "{":
                        brace_count += 1
                    elif char == "}":
                        if brace_count == 0:
                            node_end_pos = i + 1
                            break
                        brace_count -= 1

                i += 1

            if node_end_pos == -1:
                continue

            # 提取节点文本
            node_text = result_text[node_start_pos:node_end_pos]
            modified_node_text = node_text

            # 1. 删除 is_sub 字段（删除整行，包括前导空白和换行符）
            if "is_sub" in original_node_data and "is_sub" not in migrated_node_data:
                # 情况1: 有尾逗号（不是最后一个字段）
                modified_node_text = re.sub(
                    r'[ \t]*"is_sub"\s*:\s*[^,\n]+,\s*(?://[^\n]*)?\r?\n',
                    "",
                    modified_node_text,
                )
                # 情况2: 没有尾逗号（是最后一个字段），需要同时删除前一行的逗号
                modified_node_text = re.sub(
                    r',(\s*(?://[^\n]*)?\r?\n)[ \t]*"is_sub"\s*:\s*[^,\n]+\s*(?://[^\n]*)?\r?\n',
                    r"\1",
                    modified_node_text,
                )

            # 2. 删除 interrupt 字段（删除整行，包括前导空白和换行符）
            if (
                "interrupt" in original_node_data
                and "interrupt" not in migrated_node_data
            ):
                # 数组格式 interrupt: [...] （支持多行数组）
                # 情况1: 有尾逗号（不是最后一个字段）
                modified_node_text = re.sub(
                    r'[ \t]*"interrupt"\s*:\s*\[[\s\S]*?\],\s*(?://[^\n]*)?\r?\n',
                    "",
                    modified_node_text,
                )
                # 情况2: 没有尾逗号（是最后一个字段），需要同时删除前一行的逗号
                modified_node_text = re.sub(
                    r',(\s*(?://[^\n]*)?\r?\n)[ \t]*"interrupt"\s*:\s*\[[\s\S]*?\]\s*(?://[^\n]*)?\r?\n',
                    r"\1",
                    modified_node_text,
                )

                # 字符串格式 interrupt: "..."
                # 情况1: 有尾逗号（不是最后一个字段）
                modified_node_text = re.sub(
                    r'[ \t]*"interrupt"\s*:\s*"[^"]*",\s*(?://[^\n]*)?\r?\n',
                    "",
                    modified_node_text,
                )
                # 情况2: 没有尾逗号（是最后一个字段），需要同时删除前一行的逗号
                modified_node_text = re.sub(
                    r',(\s*(?://[^\n]*)?\r?\n)[ \t]*"interrupt"\s*:\s*"[^"]*"\s*(?://[^\n]*)?\r?\n',
                    r"\1",
                    modified_node_text,
                )

            # 3. 更新 next 字段
            if "next" in migrated_node_data:
                orig_next = original_node_data.get("next")
                new_next = migrated_node_data["next"]

                if orig_next != new_next:
                    if "next" in original_node_data:
                        # 替换现有的 next 字段
                        # 先找到字段所在行，获取缩进（支持字符串和数组两种格式）
                        # 尝试匹配数组格式：`"next": [...]`
                        next_line_match = re.search(
                            r'([ \t]*)"next"\s*:\s*\[[^\]]*\](,?)(\s*//[^\n]*)?',
                            modified_node_text,
                        )

                        # 如果没匹配到数组格式，尝试匹配字符串格式：`"next": "..."`
                        if not next_line_match:
                            next_line_match = re.search(
                                r'([ \t]*)"next"\s*:\s*"[^"]*"(,?)(\s*//[^\n]*)?',
                                modified_node_text,
                            )

                        if next_line_match:
                            field_indent = next_line_match.group(1)
                            trailing_comma = next_line_match.group(2)
                            inline_comment = next_line_match.group(3) or ""

                            # 生成格式化的数组值
                            new_next_str = format_array_value(
                                new_next, indent, field_indent
                            )

                            # 构建新的字段文本
                            if "\n" in new_next_str:
                                # 多行格式
                                new_field = f'{field_indent}"next": {new_next_str}{trailing_comma}{inline_comment}'
                            else:
                                # 单行格式
                                new_field = f'{field_indent}"next": {new_next_str}{trailing_comma}{inline_comment}'

                            # 替换
                            modified_node_text = (
                                modified_node_text[: next_line_match.start()]
                                + new_field
                                + modified_node_text[next_line_match.end() :]
                            )
                    else:
                        # 添加新的 next 字段
                        header_match = re.search(
                            rf'("{re.escape(node_name)}"\s*:\s*\{{\s*(?://[^\n]*)?\n)(\s+)',
                            modified_node_text,
                        )
                        if header_match:
                            insert_pos = header_match.end() - len(header_match.group(2))
                            field_indent = header_match.group(2)

                            # 生成格式化的数组值
                            new_next_str = format_array_value(
                                new_next, indent, field_indent
                            )

                            # 构建新字段
                            if "\n" in new_next_str:
                                new_field = f'{field_indent}"next": {new_next_str},\n'
                            else:
                                new_field = f'{field_indent}"next": {new_next_str},\n'

                            modified_node_text = (
                                modified_node_text[:insert_pos]
                                + new_field
                                + modified_node_text[insert_pos:]
                            )

            # 4. 更新 on_error 字段
            if "on_error" in migrated_node_data:
                orig_on_error = original_node_data.get("on_error")
                new_on_error = migrated_node_data["on_error"]

                if orig_on_error != new_on_error:
                    # 查找 on_error 字段（支持字符串和数组两种格式）
                    # 尝试匹配数组格式
                    on_error_match = re.search(
                        r'([ \t]*)"on_error"\s*:\s*\[[^\]]*\](,?)(\s*//[^\n]*)?',
                        modified_node_text,
                    )

                    # 如果没匹配到数组格式，尝试匹配字符串格式
                    if not on_error_match:
                        on_error_match = re.search(
                            r'([ \t]*)"on_error"\s*:\s*"[^"]*"(,?)(\s*//[^\n]*)?',
                            modified_node_text,
                        )

                    if on_error_match:
                        field_indent = on_error_match.group(1)
                        trailing_comma = on_error_match.group(2)
                        inline_comment = on_error_match.group(3) or ""

                        # 生成格式化的数组值
                        new_on_error_str = format_array_value(
                            new_on_error, indent, field_indent
                        )

                        # 构建新的字段文本
                        if "\n" in new_on_error_str:
                            new_field = f'{field_indent}"on_error": {new_on_error_str}{trailing_comma}{inline_comment}'
                        else:
                            new_field = f'{field_indent}"on_error": {new_on_error_str}{trailing_comma}{inline_comment}'

                        # 替换
                        modified_node_text = (
                            modified_node_text[: on_error_match.start()]
                            + new_field
                            + modified_node_text[on_error_match.end() :]
                        )

            # 替换文本中的节点
            if modified_node_text != node_text:
                # 清理多余的连续空行（最多保留一个）
                cleaned_text = re.sub(r"\n{3,}", "\n\n", modified_node_text)

                result_text = (
                    result_text[:node_start_pos]
                    + cleaned_text
                    + result_text[node_end_pos:]
                )

            # 只处理第一个匹配
            break

    return result_text


def ensure_list(value: str | list | None) -> list:
    """确保值是列表格式"""
    if value is None:
        return []
    if isinstance(value, str):
        return [value]
    return list(value)


def list_to_value(lst: list, keep_array: bool = False) -> str | list | None:
    """
    将列表转换回原始格式

    Args:
        lst: 要转换的列表
        keep_array: 是否始终保持数组格式（即使只有一个元素）

    Returns:
        如果 keep_array=True，返回列表或 None
        如果 keep_array=False，单元素返回字符串，空列表返回 None
    """
    if not lst:
        return None
    if keep_array:
        return lst
    if len(lst) == 1:
        return lst[0]
    return lst


def collect_is_sub_nodes(data: dict) -> set:
    """收集所有 is_sub: true 的节点名称"""
    is_sub_nodes = set()
    for node_name, node_data in data.items():
        if isinstance(node_data, dict) and node_data.get("is_sub", False):
            is_sub_nodes.add(node_name)
    return is_sub_nodes


JUMPBACK_PREFIX = "[JumpBack]"


def add_prefix_to_nodes(nodes: list, prefix: str) -> list:
    """给节点列表中的所有节点添加前缀"""
    return [
        f"{prefix}{node}" if not node.startswith(prefix) else node for node in nodes
    ]


def add_prefix_to_specific_nodes(nodes: list, target_nodes: set, prefix: str) -> list:
    """给节点列表中指定的节点添加前缀"""
    result = []
    for node in nodes:
        if node in target_nodes and not node.startswith(prefix):
            result.append(f"{prefix}{node}")
        else:
            result.append(node)
    return result


def migrate_node(node_data: dict, is_sub_nodes: set) -> tuple[dict, list]:
    """
    迁移单个节点的配置，保持字段顺序

    返回: (迁移后的节点数据, 更改日志列表)
    """
    if not isinstance(node_data, dict):
        return node_data, []

    changes = []

    # 使用 OrderedDict 保持字段顺序
    result = OrderedDict()

    # 收集需要处理的数据
    interrupt_list = []
    has_interrupt = "interrupt" in node_data

    if has_interrupt:
        interrupt_list = ensure_list(node_data.get("interrupt"))

    # 按原始顺序遍历字段
    for key, value in node_data.items():
        if key == "interrupt":
            # 跳过 interrupt 字段
            changes.append(f"  - 删除 interrupt: {value}")
            continue
        elif key == "is_sub":
            # 跳过 is_sub 字段
            changes.append(f"  - 删除 is_sub: {value}")
            continue
        elif key == "next":
            # 处理 next 字段
            next_list = ensure_list(value)

            # 1. 给 next 中引用 is_sub 节点的情况添加前缀
            if is_sub_nodes:
                new_next_list = add_prefix_to_specific_nodes(
                    next_list, is_sub_nodes, JUMPBACK_PREFIX
                )
                if next_list != new_next_list:
                    changes.append(
                        f"  - next 中的 is_sub 节点加 [JumpBack] 前缀: {next_list} -> {new_next_list}"
                    )
                    next_list = new_next_list

            # 2. 合并 interrupt 到 next（带 [JumpBack] 前缀）
            if interrupt_list:
                prefixed_interrupt = add_prefix_to_nodes(
                    interrupt_list, JUMPBACK_PREFIX
                )
                next_list.extend(prefixed_interrupt)
                changes.append(
                    f"  - interrupt {interrupt_list} -> 合并到 next (加 [JumpBack] 前缀): {prefixed_interrupt}"
                )

            if next_list:
                result[key] = list_to_value(next_list, keep_array=True)
            elif value is not None:
                # 原来有 next 字段但现在为空，保留空列表
                result[key] = value
        elif key == "on_error":
            # 处理 on_error 字段中引用 is_sub 节点的情况
            on_error_list = ensure_list(value)
            if is_sub_nodes:
                new_on_error_list = add_prefix_to_specific_nodes(
                    on_error_list, is_sub_nodes, JUMPBACK_PREFIX
                )
                if on_error_list != new_on_error_list:
                    changes.append(
                        f"  - on_error 中的 is_sub 节点加 [JumpBack] 前缀: {on_error_list} -> {new_on_error_list}"
                    )
                    on_error_list = new_on_error_list
            result[key] = (
                list_to_value(on_error_list, keep_array=True)
                if on_error_list
                else value
            )
        else:
            # 其他字段直接复制
            result[key] = value

    # 如果原来没有 next 字段但有 interrupt，需要创建 next 字段
    if has_interrupt and interrupt_list and "next" not in result:
        prefixed_interrupt = add_prefix_to_nodes(interrupt_list, JUMPBACK_PREFIX)
        result["next"] = list_to_value(prefixed_interrupt, keep_array=True)
        changes.append(
            f"  - interrupt {interrupt_list} -> 新建 next (加 [JumpBack] 前缀): {prefixed_interrupt}"
        )

    return result, changes


def migrate_pipeline_file(
    file_path: Path,
    global_is_sub_nodes: set,
    dry_run: bool = False,
    backup: bool = False,
) -> tuple[bool, list]:
    """
    迁移单个 pipeline JSON 文件

    Args:
        file_path: 文件路径
        global_is_sub_nodes: 全局的 is_sub 节点集合（跨所有文件收集）
        dry_run: 是否为试运行模式
        backup: 是否备份原文件

    返回: (是否有更改, 更改日志列表)
    """
    all_changes = []

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
            # 检测原文件的缩进风格
            original_indent = detect_indent(content)
            # 使用 JSONC 解析器
            data = parse_jsonc(content)
    except Exception as e:
        return False, [f"文件解析错误: {e}"]

    if not isinstance(data, dict):
        return False, ["文件内容不是 JSON 对象"]

    # 检查本文件中定义的 is_sub 节点
    local_is_sub_nodes = collect_is_sub_nodes(data)
    if local_is_sub_nodes:
        all_changes.append(f"本文件定义的 is_sub 节点: {local_is_sub_nodes}")

    # 迁移所有节点，使用全局 is_sub 节点集合
    migrated_data = OrderedDict()
    has_changes = False

    for node_name, node_data in data.items():
        migrated_node, changes = migrate_node(node_data, global_is_sub_nodes)
        migrated_data[node_name] = migrated_node

        if changes:
            has_changes = True
            all_changes.append(f"节点 [{node_name}]:")
            all_changes.extend(changes)

    if not has_changes:
        return False, []

    if not dry_run:
        # 备份原文件
        if backup:
            backup_path = file_path.with_suffix(file_path.suffix + ".bak")
            shutil.copy2(file_path, backup_path)
            all_changes.append(f"已备份到: {backup_path}")

        # 使用保留注释的方式写入迁移后的数据
        with open(file_path, "w", encoding="utf-8", newline="\n") as f:
            result_text = rebuild_json_with_comments(
                content, data, migrated_data, original_indent
            )
            f.write(result_text)
            if not result_text.endswith("\n"):
                f.write("\n")
        all_changes.append("文件已更新（保留注释并格式化）")

    return True, all_changes


def find_pipeline_files(directory: Path) -> list:
    """递归查找目录下所有 JSON 文件（排除以 . 开头的目录和文件）"""
    json_files = []

    for root, dirs, files in os.walk(directory):
        # 排除以 . 开头的目录
        dirs[:] = [d for d in dirs if not d.startswith(".")]

        for file in files:
            # 排除以 . 开头的文件
            if file.startswith("."):
                continue
            if file.lower().endswith(".json") or file.lower().endswith(".jsonc"):
                json_files.append(Path(root) / file)

    return json_files


def collect_all_is_sub_nodes(json_files: list) -> set:
    """
    扫描所有 JSON 文件，收集全局的 is_sub 节点集合

    Args:
        json_files: JSON 文件路径列表

    Returns:
        所有 is_sub: true 的节点名称集合
    """
    global_is_sub_nodes = set()

    for json_file in json_files:
        try:
            with open(json_file, "r", encoding="utf-8") as f:
                content = f.read()
                data = parse_jsonc(content)

            if isinstance(data, dict):
                is_sub_nodes = collect_is_sub_nodes(data)
                global_is_sub_nodes.update(is_sub_nodes)
        except Exception as e:
            print(f"警告: 扫描文件 {json_file} 时出错: {e}")
            continue

    return global_is_sub_nodes


def main():
    parser = argparse.ArgumentParser(
        description="将旧版 pipeline JSON 的 is_sub/interrupt 转换为 v5.1 的 [JumpBack] 前缀格式"
    )
    parser.add_argument("directory", help="包含 pipeline JSON 文件的目录路径")
    parser.add_argument(
        "--dry-run", action="store_true", help="仅显示将要进行的更改，不实际修改文件"
    )
    parser.add_argument("--backup", action="store_true", help="在修改前备份原文件")

    args = parser.parse_args()

    directory = Path(args.directory)
    if not directory.exists():
        print(f"错误: 目录不存在: {directory}")
        sys.exit(1)

    if not directory.is_dir():
        print(f"错误: 路径不是目录: {directory}")
        sys.exit(1)

    json_files = find_pipeline_files(directory)

    if not json_files:
        print(f"在目录 {directory} 中未找到 JSON 文件")
        sys.exit(0)

    print(f"找到 {len(json_files)} 个 JSON 文件")

    # 第一遍：收集全局的 is_sub 节点
    print("正在扫描所有文件，收集 is_sub 节点...")
    global_is_sub_nodes = collect_all_is_sub_nodes(json_files)

    if global_is_sub_nodes:
        print(
            f"发现 {len(global_is_sub_nodes)} 个全局 is_sub 节点: {sorted(global_is_sub_nodes)}"
        )
    else:
        print("未发现任何 is_sub 节点")

    if args.dry_run:
        print("【DRY RUN 模式 - 不会实际修改文件】\n")

    modified_count = 0

    # 第二遍：使用全局 is_sub 节点集合迁移所有文件
    for json_file in json_files:
        relative_path = (
            json_file.relative_to(directory)
            if json_file.is_relative_to(directory)
            else json_file
        )

        has_changes, changes = migrate_pipeline_file(
            json_file, global_is_sub_nodes, args.dry_run, args.backup
        )

        if has_changes:
            modified_count += 1
            print(f"\n{'=' * 60}")
            print(f"文件: {relative_path}")
            print("-" * 60)
            for change in changes:
                print(change)

    print(f"\n{'=' * 60}")
    print(f"总计: {modified_count}/{len(json_files)} 个文件需要迁移")

    if args.dry_run and modified_count > 0:
        print("\n提示: 使用不带 --dry-run 参数运行以实际执行迁移")


if __name__ == "__main__":
    main()
