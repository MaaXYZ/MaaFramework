#!/usr/bin/env python3
"""校验 interface.schema.json 声明的可导入字段与 interface_import.schema.json 的顶层字段一致。

import 文件的 schema 顶层是 additionalProperties: false，因此一旦协议新增了可导入字段
（例如 v2.4.0 的 group）却没同步到这里，合法的导入片段就会被判成非法。
用法: python tools/check_interface_schema.py [--verbose]
"""

import argparse
import json
import pathlib
import re
import sys

TOOLS_DIR = pathlib.Path(__file__).resolve().parent
MAIN_SCHEMA_PATH = TOOLS_DIR / "interface.schema.json"
IMPORT_SCHEMA_PATH = TOOLS_DIR / "interface_import.schema.json"

DECLARATION_PREFIX = "支持导入这些文件中的"
DECLARATION_SUFFIX = "字段"
FIELD_PATTERN = re.compile(r"^[a-z_][a-z0-9_]*$")


def declared_importable_fields(main_schema):
    """从主 schema 的 import 描述里取回可导入字段清单。"""
    description = main_schema["properties"]["import"]["description"]
    start = description.find(DECLARATION_PREFIX)
    if start < 0:
        raise SystemExit(f"{MAIN_SCHEMA_PATH.name}: import 描述里找不到「{DECLARATION_PREFIX}」，请同步更新本脚本")
    start += len(DECLARATION_PREFIX)

    end = description.find(DECLARATION_SUFFIX, start)
    if end < 0:
        raise SystemExit(f"{MAIN_SCHEMA_PATH.name}: import 描述里找不到「{DECLARATION_SUFFIX}」，请同步更新本脚本")

    # 去掉「（💡 v2.4.0）」这类版本标注，再按中文/英文枚举分隔符切分
    text = re.sub(r"（[^）]*）|\([^)]*\)", "", description[start:end])
    fields = set()
    for token in re.split(r"[、,，]|与|\s+", text):
        token = token.strip().strip("`*")
        if FIELD_PATTERN.match(token):
            fields.add(token)

    if not fields:
        raise SystemExit(f"{MAIN_SCHEMA_PATH.name}: import 描述里没解析出任何字段，请同步更新本脚本")
    return fields


def accepted_top_level_fields(import_schema):
    return set(import_schema["properties"].keys())


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--verbose", action="store_true", help="打印两边解析出的字段清单")
    args = parser.parse_args()

    main_schema = json.loads(MAIN_SCHEMA_PATH.read_text(encoding="utf-8"))
    import_schema = json.loads(IMPORT_SCHEMA_PATH.read_text(encoding="utf-8"))

    declared = declared_importable_fields(main_schema)
    accepted = accepted_top_level_fields(import_schema)

    if args.verbose:
        print(f"declared ({MAIN_SCHEMA_PATH.name} import 描述): {', '.join(sorted(declared))}")
        print(f"accepted ({IMPORT_SCHEMA_PATH.name} 顶层): {', '.join(sorted(accepted))}")

    missing = sorted(declared - accepted)
    extra = sorted(accepted - declared)
    if not missing and not extra:
        print(f"OK: {len(accepted)} 个可导入字段一致: {', '.join(sorted(accepted))}")
        return 0

    for field in missing:
        print(f"ERROR: 声明可导入的字段 `{field}` 不在 {IMPORT_SCHEMA_PATH.name} 顶层，"
              f"其 additionalProperties: false 会拒绝合法片段")
    for field in extra:
        print(f"ERROR: {IMPORT_SCHEMA_PATH.name} 顶层字段 `{field}` 未出现在 "
              f"{MAIN_SCHEMA_PATH.name} 的 import 描述里")
    return 1


if __name__ == "__main__":
    sys.exit(main())
