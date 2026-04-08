#!/usr/bin/env python3
"""生成 struct_to_tuple 代码并注入到 BitstreamSerializer.hpp"""

def generate_struct_to_tuple(start_n, end_n):
    """生成 struct_to_tuple 的 else if constexpr 分支"""
    lines = []
    for n in range(start_n, end_n + 1):
        params = ", ".join([f"m{i}" for i in range(1, n + 1)])
        binding = f"const auto [{params}] = obj;"
        ret = f"return std::make_tuple({params});"
        
        lines.append(f"    }} else if constexpr (N == {n}) {{")
        lines.append(f"        {binding} {ret}")
    
    return "\n".join(lines)

def inject_code(file_path):
    """将生成的代码注入到文件中"""
    # 读取文件
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 找到需要替换的位置
    # 旧的 else 分支（N == 16 之后）
    old_else_block = """    } else {
        static_assert(N <= 16, "struct_to_tuple supports up to 16 fields. Add more branches to support more!");
        return std::make_tuple();
    }"""
    
    # 生成新的代码（N=17 到 N=64）
    new_branches = generate_struct_to_tuple(17, 64)
    
    # 新的 else 分支
    new_else_block = new_branches + """
    } else {
        static_assert(N <= 64, "struct_to_tuple supports up to 64 fields. Add more branches to support more!");
        return std::make_tuple();
    }"""
    
    # 替换
    new_content = content.replace(old_else_block, new_else_block)
    
    # 写回文件
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print("✅ 成功注入代码到 BitstreamSerializer.hpp")

if __name__ == "__main__":
    file_path = "/home/ww/codings/rpl/include/RPL/Meta/BitstreamSerializer.hpp"
    inject_code(file_path)
