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
    3. 删除 is_sub 和 interrupt 字段

特性:
    - 支持 JSONC（带注释的 JSON，包括 // 和 /* */ 两种注释）
    - 保持 JSON 字段顺序不变
    - 保持原文件的缩进风格
"""

import re
import os
import sys
import shutil
import argparse
from pathlib import Path
from typing import Any
from collections import OrderedDict


def remove_jsonc_comments(text: str) -> str:
    """
    移除 JSONC 文件中的注释（支持 // 和 /* */ 两种注释）
    同时保留字符串内的内容不受影响
    """
    result = []
    i = 0
    in_string = False
    string_char = None

    while i < len(text):
        # 处理字符串内容
        if in_string:
            if text[i] == "\\" and i + 1 < len(text):
                # 转义字符，跳过下一个字符
                result.append(text[i : i + 2])
                i += 2
                continue
            elif text[i] == string_char:
                # 字符串结束
                in_string = False
                string_char = None
                result.append(text[i])
                i += 1
                continue
            else:
                result.append(text[i])
                i += 1
                continue

        # 检查字符串开始
        if text[i] in ('"', "'"):
            in_string = True
            string_char = text[i]
            result.append(text[i])
            i += 1
            continue

        # 检查单行注释 //
        if text[i : i + 2] == "//":
            # 跳过到行尾
            while i < len(text) and text[i] != "\n":
                i += 1
            continue

        # 检查多行注释 /* */
        if text[i : i + 2] == "/*":
            i += 2
            while i < len(text) and text[i : i + 2] != "*/":
                i += 1
            i += 2  # 跳过 */
            continue

        result.append(text[i])
        i += 1

    return "".join(result)


def parse_jsonc(text: str) -> Any:
    """解析 JSONC 文件内容，返回 OrderedDict 以保持字段顺序"""
    import json

    clean_text = remove_jsonc_comments(text)
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


def dump_json_ordered(data: Any, indent: str = "    ") -> str:
    """
    将数据序列化为 JSON 字符串，保持字段顺序
    """
    import json

    def convert_to_serializable(obj):
        if isinstance(obj, OrderedDict):
            return {k: convert_to_serializable(v) for k, v in obj.items()}
        elif isinstance(obj, dict):
            return {k: convert_to_serializable(v) for k, v in obj.items()}
        elif isinstance(obj, list):
            return [convert_to_serializable(item) for item in obj]
        else:
            return obj

    serializable_data = convert_to_serializable(data)

    # 计算缩进空格数
    indent_size = len(indent.replace("\t", "    "))

    return json.dumps(serializable_data, ensure_ascii=False, indent=indent_size)


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


def migrate_node(
    node_name: str, node_data: dict, is_sub_nodes: set
) -> tuple[dict, list]:
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
    has_is_sub = "is_sub" in node_data

    if has_interrupt:
        interrupt_list = ensure_list(node_data.get("interrupt"))

    # 按原始顺序遍历字段
    for key, value in node_data.items():
        if key == "interrupt":
            # 跳过 interrupt 字段，稍后处理
            continue
        elif key == "is_sub":
            # 跳过 is_sub 字段
            is_sub_value = value
            changes.append(f"  - 删除 is_sub: {is_sub_value}")
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
    file_path: Path, dry_run: bool = False, backup: bool = False
) -> tuple[bool, list]:
    """
    迁移单个 pipeline JSON 文件

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

    # 第一遍：收集所有 is_sub 节点
    is_sub_nodes = collect_is_sub_nodes(data)
    if is_sub_nodes:
        all_changes.append(f"发现 is_sub 节点: {is_sub_nodes}")

    # 第二遍：迁移所有节点，保持顺序
    migrated_data = OrderedDict()
    has_changes = False

    for node_name, node_data in data.items():
        migrated_node, changes = migrate_node(node_name, node_data, is_sub_nodes)
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

        # 写入迁移后的数据，保持原文件的缩进风格
        with open(file_path, "w", encoding="utf-8", newline="\n") as f:
            json_str = dump_json_ordered(migrated_data, original_indent)
            f.write(json_str)
            f.write("\n")  # 文件末尾添加换行
        all_changes.append("文件已更新")

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
            if file.lower().endswith(".json"):
                json_files.append(Path(root) / file)

    return json_files


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
    if args.dry_run:
        print("【DRY RUN 模式 - 不会实际修改文件】\n")

    modified_count = 0

    for json_file in json_files:
        relative_path = (
            json_file.relative_to(directory)
            if json_file.is_relative_to(directory)
            else json_file
        )

        has_changes, changes = migrate_pipeline_file(
            json_file, args.dry_run, args.backup
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
