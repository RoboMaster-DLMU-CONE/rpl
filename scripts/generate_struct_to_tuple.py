#!/usr/bin/env python3
"""生成 struct_to_tuple 代码并注入到 BitstreamSerializer.hpp"""

from __future__ import annotations

import argparse
from pathlib import Path

FUNCTION_SIGNATURE = (
    "template <std::size_t N, typename T> constexpr auto struct_to_tuple(const T &obj) {"
)


def generate_struct_to_tuple_function(max_fields: int) -> str:
    """生成完整的 struct_to_tuple 函数实现（支持 1..max_fields）。"""
    lines: list[str] = [FUNCTION_SIGNATURE]
    for n in range(1, max_fields + 1):
        members = ", ".join(f"m{i}" for i in range(1, n + 1))
        tuple_items = ", ".join(f"to_array_if_needed(m{i})" for i in range(1, n + 1))
        keyword = "if" if n == 1 else "else if"
        lines.append(
            f"  {keyword} constexpr (N == {n}) {{ const auto [{members}] = obj; "
            f"return std::make_tuple({tuple_items}); }}"
        )
    lines.extend(
        [
            "  else {",
            f"    static_assert(N <= {max_fields}, "
            f"\"struct_to_tuple supports up to {max_fields} fields. Add more \"",
            "                           \"branches to support more!\");",
            "    return std::make_tuple();",
            "  }",
            "}",
        ]
    )
    return "\n".join(lines)


def find_function_bounds(content: str) -> tuple[int, int]:
    """找到 struct_to_tuple 函数的完整范围 [start, end)。"""
    start = content.find(FUNCTION_SIGNATURE)
    if start == -1:
        raise ValueError("未找到 struct_to_tuple 函数签名")

    open_brace = content.find("{", start)
    if open_brace == -1:
        raise ValueError("未找到 struct_to_tuple 函数体起始大括号")

    depth = 0
    for idx in range(open_brace, len(content)):
        if content[idx] == "{":
            depth += 1
        elif content[idx] == "}":
            depth -= 1
            if depth == 0:
                return start, idx + 1
    raise ValueError("未找到 struct_to_tuple 函数体结束大括号")


def inject_code(file_path: Path, max_fields: int) -> None:
    """将生成的代码注入到文件中。"""
    content = file_path.read_text(encoding="utf-8")
    start, end = find_function_bounds(content)
    new_function = generate_struct_to_tuple_function(max_fields)
    new_content = content[:start] + new_function + content[end:]
    file_path.write_text(new_content, encoding="utf-8")
    print(f"✅ 成功注入代码到 {file_path}，当前支持上限: {max_fields}")


def main() -> None:
    parser = argparse.ArgumentParser(description="生成并注入 struct_to_tuple 分支")
    parser.add_argument(
        "--file",
        type=Path,
        default=Path("/home/ww/codings/rpl/include/RPL/Meta/BitstreamSerializer.hpp"),
        help="目标 BitstreamSerializer.hpp 路径",
    )
    parser.add_argument(
        "--max-fields",
        type=int,
        default=64,
        help="生成分支上限（从 1 开始）",
    )
    args = parser.parse_args()

    if args.max_fields < 1:
        raise ValueError("--max-fields 必须 >= 1")

    inject_code(args.file, args.max_fields)


if __name__ == "__main__":
    main()
