#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
解析 MAA 日志文件，统计每个函数打印的日志数量并排序输出
"""

import re
import sys
from collections import Counter
from pathlib import Path


def extract_function_name(line):
    """
    从日志行中提取函数名
    格式: [时间戳][级别][Px...][Tx...][文件名][行号][函数名] 其他内容
    """
    # 匹配所有方括号中的内容
    pattern = r'\[([^\]]+)\]'
    matches = re.findall(pattern, line)
    
    if len(matches) >= 7:
        # 函数名是第7个方括号中的内容（索引6）
        # 格式: [时间戳][级别][Px...][Tx...][文件名][行号][函数名]
        function_name = matches[6]
        return function_name
    elif len(matches) >= 1:
        # 如果格式不标准，可能没有函数名，返回 None
        return None
    
    return None


def analyze_log_file(log_file_path):
    """
    分析日志文件，统计函数调用次数
    """
    function_counter = Counter()
    total_lines = 0
    parsed_lines = 0
    
    log_path = Path(log_file_path)
    if not log_path.exists():
        print(f"错误: 日志文件不存在: {log_file_path}")
        return None
    
    print(f"正在分析日志文件: {log_file_path}")
    print("=" * 60)
    
    try:
        with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                total_lines += 1
                function_name = extract_function_name(line.strip())
                if function_name:
                    function_counter[function_name] += 1
                    parsed_lines += 1
        
        print(f"总日志行数: {total_lines}")
        print(f"成功解析行数: {parsed_lines}")
        print(f"统计到的函数数: {len(function_counter)}")
        print("=" * 60)
        print()
        
        return function_counter
    
    except Exception as e:
        print(f"读取文件时出错: {e}")
        return None


def print_results(function_counter, top_n=None):
    """
    打印统计结果，按次数降序排序
    """
    if not function_counter:
        print("没有找到任何函数调用记录")
        return
    
    # 按次数降序排序
    sorted_functions = sorted(function_counter.items(), key=lambda x: x[1], reverse=True)
    
    if top_n:
        sorted_functions = sorted_functions[:top_n]
    
    print(f"{'排名':<6} {'调用次数':<12} {'函数名'}")
    print("-" * 80)
    
    for rank, (function_name, count) in enumerate(sorted_functions, 1):
        print(f"{rank:<6} {count:<12} {function_name}")


def main():
    """
    主函数
    """
    # 默认使用 maa.log，也可以通过命令行参数指定
    if len(sys.argv) > 1:
        log_file = sys.argv[1]
    else:
        log_file = "maa.log"
    
    # 可选：只显示前 N 个结果
    top_n = None
    if len(sys.argv) > 2:
        try:
            top_n = int(sys.argv[2])
        except ValueError:
            print(f"警告: 无效的 top_n 参数: {sys.argv[2]}，将显示所有结果")
    
    function_counter = analyze_log_file(log_file)
    
    if function_counter:
        print_results(function_counter, top_n)


if __name__ == "__main__":
    main()

