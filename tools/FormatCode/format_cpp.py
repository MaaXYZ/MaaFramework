#!/usr/bin/env python3
"""
格式化整个项目中的 C++ 文件（排除 3rdparty 目录）
使用根目录下的 .clang-format 配置文件
"""

import subprocess
import sys
from pathlib import Path


def find_cpp_files(root_dir, exclude_dirs):
    """
    查找所有 C++ 文件，排除指定目录

    Args:
        root_dir: 根目录路径
        exclude_dirs: 要排除的目录列表（相对于 root_dir）

    Returns:
        C++ 文件路径列表
    """
    root_path = Path(root_dir).resolve()
    exclude_paths = [root_path / d for d in exclude_dirs]

    cpp_extensions = {
        ".cpp",
        ".hpp",
        ".h",
        ".cc",
        ".cxx",
        ".c++",
        ".h++",
        ".hh",
        ".hxx",
        ".cppm",
    }
    cpp_files = []

    for file_path in root_path.rglob("*"):
        # 只处理文件，跳过目录
        if not file_path.is_file():
            continue

        # 跳过排除的目录及其子目录中的文件
        try:
            file_path_resolved = file_path.resolve()
            if any(
                exclude_path.resolve() in file_path_resolved.parents
                for exclude_path in exclude_paths
            ):
                continue
        except (OSError, ValueError):
            # 如果路径解析失败（例如符号链接问题），跳过
            continue

        # 检查是否是 C++ 文件
        if file_path.suffix in cpp_extensions:
            cpp_files.append(file_path)

    return sorted(cpp_files)


def format_file(file_path, clang_format_cmd="clang-format", dry_run=False):
    """
    使用 clang-format 格式化单个文件

    Args:
        file_path: 要格式化的文件路径
        clang_format_cmd: clang-format 命令
        dry_run: 如果为 True，只检查格式而不修改文件

    Returns:
        (success, message) 元组
    """
    try:
        if dry_run:
            # 检查格式，不修改文件
            result = subprocess.run(
                [clang_format_cmd, "--dry-run", "--Werror", str(file_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode == 0:
                return True, "格式正确"
            else:
                return False, result.stderr.strip() or "格式需要调整"
        else:
            # 格式化文件
            result = subprocess.run(
                [clang_format_cmd, "-i", str(file_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode == 0:
                return True, "格式化成功"
            else:
                return False, result.stderr.strip() or "格式化失败"
    except FileNotFoundError:
        return (
            False,
            f"未找到 {clang_format_cmd}，请确保已安装 clang-format 并在 PATH 中",
        )
    except Exception as e:
        return False, f"错误: {str(e)}"


def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(
        description="格式化项目中的 C++ 文件（排除 3rdparty 目录）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                    # 格式化所有文件
  %(prog)s --dry-run          # 只检查格式，不修改文件
  %(prog)s --clang-format clang-format-17  # 使用特定版本的 clang-format
        """,
    )
    parser.add_argument("--dry-run", action="store_true", help="只检查格式，不修改文件")
    parser.add_argument(
        "--clang-format",
        default="clang-format",
        help="clang-format 命令（默认: clang-format）",
    )
    parser.add_argument(
        "--exclude",
        nargs="+",
        default=["3rdparty"],
        help="要排除的目录（默认: 3rdparty）",
    )
    parser.add_argument(
        "--root",
        type=str,
        default=None,
        help="项目根目录（默认: 脚本所在目录的父目录）",
    )
    parser.add_argument("--verbose", action="store_true", help="显示详细信息")

    args = parser.parse_args()

    # 确定根目录
    if args.root:
        root_dir = Path(args.root).resolve()
    else:
        # 默认使用脚本所在目录的父目录（项目根目录）
        root_dir = Path(__file__).parent.parent.parent.resolve()

    # 检查 .clang-format 文件是否存在
    clang_format_config = root_dir / ".clang-format"
    if not clang_format_config.exists():
        print(
            f"错误: 未找到 .clang-format 文件: {clang_format_config}", file=sys.stderr
        )
        sys.exit(1)

    if args.verbose:
        print(f"项目根目录: {root_dir}")
        print(f"配置文件: {clang_format_config}")
        print(f"排除目录: {args.exclude}")
        print(f"clang-format 命令: {args.clang_format}")
        print()

    # 查找所有 C++ 文件
    print("正在查找 C++ 文件...")
    cpp_files = find_cpp_files(root_dir, args.exclude)

    if not cpp_files:
        print("未找到任何 C++ 文件")
        return

    print(f"找到 {len(cpp_files)} 个 C++ 文件")
    print()

    # 格式化文件
    success_count = 0
    fail_count = 0
    need_format_count = 0

    for i, file_path in enumerate(cpp_files, 1):
        rel_path = file_path.relative_to(root_dir)
        if args.verbose or args.dry_run:
            print(f"[{i}/{len(cpp_files)}] {rel_path}...", end=" ", flush=True)

        success, message = format_file(file_path, args.clang_format, args.dry_run)

        if success:
            if args.dry_run:
                if args.verbose:
                    print("✓", message)
                # 在 dry-run 模式下，如果格式正确，不显示
            else:
                if args.verbose:
                    print("✓", message)
                success_count += 1
        else:
            if args.dry_run:
                # 在 dry-run 模式下，格式不正确意味着需要格式化
                need_format_count += 1
                print("✗ 需要格式化")
            else:
                print(f"✗ {message}", file=sys.stderr)
                fail_count += 1

    # 输出统计信息
    print()
    if args.dry_run:
        print(f"检查完成: {len(cpp_files)} 个文件")
        if need_format_count > 0:
            print(f"需要格式化的文件: {need_format_count}")
            sys.exit(1)
        else:
            print("所有文件格式正确")
    else:
        print(f"格式化完成: {success_count} 个成功, {fail_count} 个失败")


if __name__ == "__main__":
    main()
