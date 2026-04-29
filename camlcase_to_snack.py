#!/usr/bin/env python3
# camel_to_snake_enhanced.py
import os
import re
import sys

def camel_to_snake(name):
    """增强的驼峰转蛇形"""
    if not name or '_' in name:
        return name
    
    # 特殊情况处理
    special_cases = {
        'UEFI': 'uefi',
        'GUID': 'guid',
        'UUID': 'uuid',
        'HTML': 'html',
        'XML': 'xml',
        'CPU': 'cpu',
        'IO': 'io',
        'PCI': 'pci',
        'ACPI': 'acpi',
        'APIC': 'apic',
        'MMIO': 'mmio',
        'IRQ': 'irq',
        'IDT': 'idt',
        'GDT': 'gdt',
        'TSS': 'tss',
        'CR3': 'cr3',
        'IP': 'ip',
        'TCP': 'tcp',
        'UDP': 'udp',
        'MAC': 'mac',
        'VLAN': 'vlan',
    }
    
    # 先处理已知的缩写
    for abbrev, replacement in special_cases.items():
        if abbrev in name:
            # 但要注意：UEFITable 应该变成 uefi_table，而不是 uefit_able
            pass
    
    # 标准转换
    # 1. 在小写字母和大写字母之间插入下划线
    s1 = re.sub(r'([a-z])([A-Z])', r'\1_\2', name)
    # 2. 在大写字母和小写字母之间插入下划线（处理连续大写）
    s2 = re.sub(r'([A-Z])([A-Z][a-z])', r'\1_\2', s1)
    
    # 转换为小写
    result = s2.lower()
    
    # 特殊处理：保留某些模式
    # 如：sub_create_process_ex_w -> sub_create_process_ex_w (保持ex_w)
    
    return result

def process_identifier_in_line(line, identifier):
    """在行中处理标识符"""
    snake = camel_to_snake(identifier)
    
    # 替换整个单词，避免部分匹配
    # 使用单词边界，但注意边界可能包括 . -> 等
    pattern = r'\b' + re.escape(identifier) + r'\b'
    
    # 特殊情况：结构体成员访问 process.subProcess
    pattern2 = r'([.>])' + re.escape(identifier) + r'\b'
    
    new_line = re.sub(pattern, snake, line)
    new_line = re.sub(pattern2, r'\1' + snake, new_line)
    
    return new_line

def process_file_enhanced(filepath):
    """增强的文件处理"""
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    new_lines = []
    changes = []
    
    for i, line in enumerate(lines):
        new_line = line
        
        # 跳过字符串和注释
        if '//' in line or '/*' in line or '"' in line or "'" in line:
            # 简单处理：不转换包含字符串/注释的行
            # 更好的方法是解析，但这里简化
            new_lines.append(new_line)
            continue
        
        # 查找可能的驼峰标识符
        # 模式1: 标准驼峰 (subCreateProcess)
        pattern1 = r'\b([a-z][a-z0-9]*[A-Z][a-zA-Z0-9]+)\b'
        matches1 = re.findall(pattern1, new_line)
        
        # 模式2: 帕斯卡 (SubCreateProcess)
        pattern2 = r'\b([A-Z][a-z0-9]*[A-Z][a-zA-Z0-9]+)\b'
        matches2 = re.findall(pattern2, new_line)
        
        all_matches = list(set(matches1 + matches2))
        
        for match in all_matches:
            if len(match) < 4:  # 太短可能是误报
                continue
            
            snake = camel_to_snake(match)
            if snake != match.lower():  # 确实需要转换
                # 检查是否在常见不应该转换的列表中
                skip_list = ['malloc', 'free', 'printf', 'sizeof', 'typeof']
                if match in skip_list:
                    continue
                
                # 替换
                new_line = process_identifier_in_line(new_line, match)
                changes.append((i+1, match, snake))
        
        new_lines.append(new_line)
    
    # 如果有变化，写入文件
    if changes:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        
        print(f"修改: {filepath}")
        for line_num, old, new in changes[:5]:  # 只显示前5个
            print(f"  行 {line_num}: {old} -> {new}")
        if len(changes) > 5:
            print(f"  ... 还有 {len(changes)-5} 个修改")
        
        return True
    
    return False

def main():
    if len(sys.argv) < 2:
        root_dir = "."
    else:
        root_dir = sys.argv[1]
    
    print("增强版驼峰转蛇形转换")
    print(f"目录: {root_dir}")
    print("-" * 60)
    
    changed_files = 0
    
    for root, dirs, files in os.walk(root_dir):
        # 跳过隐藏目录
        dirs[:] = [d for d in dirs if not d.startswith('.')]
        
        for file in files:
            if file.endswith(('.c', '.h')):
                filepath = os.path.join(root, file)
                if process_file_enhanced(filepath):
                    changed_files += 1
    
    print("-" * 60)
    print(f"完成！修改了 {changed_files} 个文件")
    
    # 运行诊断检查剩余
    print("\n检查剩余驼峰命名...")
    os.system(f"python3 diagnose_camel.py {root_dir}")

if __name__ == "__main__":
    main()
