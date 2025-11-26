# E-comOS - 中断服务例程(ISR)汇编代码

.section .text

# 无错误码的中断宏
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli                    # 禁用中断
    push $0                # 推入虚拟错误码
    push $\num             # 推入中断号
    jmp isr_common_stub    # 跳转到通用处理程序
.endm

# 有错误码的中断宏
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli                    # 禁用中断
    push $\num             # 推入中断号
    jmp isr_common_stub    # 跳转到通用处理程序
.endm

# CPU异常处理程序
ISR_NOERRCODE 0   # 除零错误
ISR_NOERRCODE 1   # 调试异常
ISR_NOERRCODE 2   # 不可屏蔽中断
ISR_NOERRCODE 3   # 断点异常
ISR_NOERRCODE 4   # 溢出异常
ISR_NOERRCODE 5   # 边界检查异常
ISR_NOERRCODE 6   # 无效操作码异常
ISR_NOERRCODE 7   # 设备不可用异常
ISR_ERRCODE   8   # 双重故障
ISR_NOERRCODE 9   # 协处理器段溢出
ISR_ERRCODE   10  # 无效TSS异常
ISR_ERRCODE   11  # 段不存在异常
ISR_ERRCODE   12  # 栈故障异常
ISR_ERRCODE   13  # 一般保护异常
ISR_ERRCODE   14  # 页故障异常
ISR_NOERRCODE 15  # 保留
ISR_NOERRCODE 16  # 浮点异常
ISR_ERRCODE   17  # 对齐检查异常
ISR_NOERRCODE 18  # 机器检查异常
ISR_NOERRCODE 19  # SIMD浮点异常
ISR_NOERRCODE 20  # 虚拟化异常

# 保留的异常处理程序
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

# 通用中断处理程序
isr_common_stub:
    pusha                  # 保存所有通用寄存器
    
    mov %ds, %ax           # 保存数据段描述符
    push %eax
    
    mov $0x10, %ax         # 加载内核数据段描述符
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    call isr_handler       # 调用C语言中断处理程序
    
    pop %eax               # 恢复原始数据段描述符
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    popa                   # 恢复所有通用寄存器
    add $8, %esp           # 清理推入的错误码和中断号
    sti                    # 重新启用中断
    iret                   # 从中断返回