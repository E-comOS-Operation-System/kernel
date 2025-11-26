# E-comOS - 硬件中断请求(IRQ)汇编代码

.section .text

# IRQ处理程序宏
.macro IRQ num, isr_num
.global irq\num
irq\num:
    cli
    push $0                # 虚拟错误码
    push $\isr_num         # IRQ号
    jmp irq_common_stub
.endm

# 硬件中断处理程序 (IRQ 0-15)
IRQ 0, 32    # 系统定时器
IRQ 1, 33    # 键盘
IRQ 2, 34    # 级联中断
IRQ 3, 35    # 串口2
IRQ 4, 36    # 串口1
IRQ 5, 37    # 并口2
IRQ 6, 38    # 软盘驱动器
IRQ 7, 39    # 并口1
IRQ 8, 40    # 实时时钟
IRQ 9, 41    # 自由使用
IRQ 10, 42   # 自由使用
IRQ 11, 43   # 自由使用
IRQ 12, 44   # PS/2鼠标
IRQ 13, 45   # 数学协处理器
IRQ 14, 46   # 主IDE
IRQ 15, 47   # 从IDE

# 通用IRQ处理程序
irq_common_stub:
    pusha                  # 保存所有通用寄存器
    
    mov %ds, %ax           # 保存数据段描述符
    push %eax
    
    mov $0x10, %ax         # 加载内核数据段描述符
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    call irq_handler       # 调用C语言IRQ处理程序
    
    pop %eax               # 恢复原始数据段描述符
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    popa                   # 恢复所有通用寄存器
    add $8, %esp           # 清理推入的错误码和中断号
    sti                    # 重新启用中断
    iret                   # 从中断返回