.section .text.boot
.global _start

_start:
    /* SP設定してCへ */
    ldr x0, =__stack_top
    mov sp, x0
    bl  main

1:  wfi
    b   1b

