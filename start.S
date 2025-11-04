/* start.S */
.section .text.boot
.global _start

/* ---------------- 例外ベクタ（EL1） ---------------- */
.section .vectors, "ax"
.align 11
.global vector_table_el1
vector_table_el1:
    b .                /* sync_el1t */
    .balign 128, 0xd503201f
    b .                /* irq_el1t  */
    .balign 128, 0xd503201f
    b .                /* fiq_el1t  */
    .balign 128, 0xd503201f
    b .                /* serr_el1t */
    .balign 128, 0xd503201f
    b .                /* sync_el1h */
    .balign 128, 0xd503201f
    b irq_el1h         /* irq_el1h  ←ここだけ使う */
    .balign 128, 0xd503201f
    b .                /* fiq_el1h  */
    .balign 128, 0xd503201f
    b .                /* serr_el1h */
    .balign 128, 0xd503201f
    /* 残りは未使用 */
    .rept 8
    b .
    .balign 128, 0xd503201f
    .endr

.extern irq_handler

irq_el1h:
    sub   sp, sp, #0x100
    stp   x0,  x1,  [sp, #0x00]
    stp   x2,  x3,  [sp, #0x10]
    stp   x4,  x5,  [sp, #0x20]
    stp   x6,  x7,  [sp, #0x30]
    stp   x8,  x9,  [sp, #0x40]
    stp   x10, x11, [sp, #0x50]
    stp   x12, x13, [sp, #0x60]
    stp   x14, x15, [sp, #0x70]
    stp   x16, x17, [sp, #0x80]
    stp   x18, x19, [sp, #0x90]
    stp   x20, x21, [sp, #0xA0]
    stp   x22, x23, [sp, #0xB0]
    stp   x24, x25, [sp, #0xC0]
    stp   x26, x27, [sp, #0xD0]
    stp   x28, x29, [sp, #0xE0]
    str   x30,        [sp, #0xF8]
    bl    irq_handler
    ldr   x30,        [sp, #0xF8]
    ldp   x28, x29, [sp, #0xE0]
    ldp   x26, x27, [sp, #0xD0]
    ldp   x24, x25, [sp, #0xC0]
    ldp   x22, x23, [sp, #0xB0]
    ldp   x20, x21, [sp, #0xA0]
    ldp   x18, x19, [sp, #0x90]
    ldp   x16, x17, [sp, #0x80]
    ldp   x14, x15, [sp, #0x70]
    ldp   x12, x13, [sp, #0x60]
    ldp   x10, x11, [sp, #0x50]
    ldp   x8,  x9,  [sp, #0x40]
    ldp   x6,  x7,  [sp, #0x30]
    ldp   x4,  x5,  [sp, #0x20]
    ldp   x2,  x3,  [sp, #0x10]
    ldp   x0,  x1,  [sp, #0x00]
    add   sp, sp, #0x100
    eret

/* ---------------- エントリ ---------------- */
.section .text.boot
_start:
    /* 暫定SP */
    ldr   x0, =__stack_top
    mov   sp, x0

    /* 現在EL */
    mrs   x0, CurrentEL
    lsr   x0, x0, #2          // x0 = 1,2,3

    /* EL3 -> EL2 */
    cmp   x0, #3
    b.ne  1f
    mov   x1, #(1<<0)         // SCR_EL3.NS=1
    orr   x1, x1, #(1<<10)    // SCR_EL3.RW=1 (EL2 AArch64)
    msr   SCR_EL3, x1
    mov   x1, #0x3C9          // SPSR_EL3: EL2h, DAIF=masked
    msr   SPSR_EL3, x1
    ldr   x1, =_enter_el2
    msr   ELR_EL3, x1
    eret

1:  /* EL2 -> EL1 */
    cmp   x0, #2
    b.ne  2f
_enter_el2:
    mov   x1, #(1<<31)        // HCR_EL2.RW=1 (EL1 AArch64)
    msr   HCR_EL2, x1
    mov   x1, #3              // CNTHCTL_EL2: EL1PCEN=1|EL1PCTEN=1
    msr   CNTHCTL_EL2, x1
    msr   CNTVOFF_EL2, xzr
    isb
    mov   x1, #0x3C5          // SPSR_EL2: EL1h, DAIF=masked
    msr   SPSR_EL2, x1
    ldr   x1, =_enter_el1
    msr   ELR_EL2, x1
    eret

2:
_enter_el1:
    /* ベクタ */
    ldr   x1, =vector_table_el1
    msr   VBAR_EL1, x1
    isb

    /* SP を EL1 用に */
    mov   x0, #1
    msr   SPSel, x0
    ldr   x0, =__stack_top
    mov   sp, x0

    bl    main

halt:
    wfi
    b halt

