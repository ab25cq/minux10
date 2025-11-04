.section .text.boot
.global _start

_start:
    // 物理アドレス0x80000に配置される
    // スタックを設定
    ldr x0, =_stack_top
    mov sp, x0

    // main() 呼び出し
    bl main

1:  wfi        // 無限待機ループ
    b 1b

.section .bss
.space 4096
_stack_top:

