/* start.S */
.section .text.boot
.global _start

/* -------- 例外ベクタ（EL1） -------- */
.section .vectors, "ax"
.align 11
.global vector_table_el1
vector_table_el1:
    b sync_el1t
    .balign 128, 0xd503201f
    b irq_el1t
    .balign 128, 0xd503201f
    b fiq_el1t
    .balign 128, 0xd503201f
    b serr_el1t
    .balign 128, 0xd503201f
    b sync_el1h
    .balign 128, 0xd503201f
    b irq_el1h
    .balign 128, 0xd503201f
    b fiq_el1h
    .balign 128, 0xd503201f
    b serr_el1h
    .balign 128, 0xd503201f
    b sync_el0_64
    .balign 128, 0xd503201f
    b irq_el0_64
    .balign 128, 0xd503201f
    b fiq_el0_64
    .balign 128, 0xd503201f
    b serr_el0_64
    .balign 128, 0xd503201f
    b sync_el0_32
    .balign 128, 0xd503201f
    b irq_el0_32
    .balign 128, 0xd503201f
    b fiq_el0_32
    .balign 128, 0xd503201f
    b serr_el0_32
    .balign 128, 0xd503201f

.extern irq_handler

sync_el1t:
sync_el1h:
sync_el0_64:
sync_el0_32:
fiq_el1t:
fiq_el1h:
fiq_el0_64:
fiq_el0_32:
serr_el1t:
serr_el1h:
serr_el0_64:
serr_el0_32:
irq_el1t:
irq_el0_64:
irq_el0_32:
default_trap:
    b default_trap

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

/* -------- エントリ -------- */
.section .text.boot
_start:
    /* 一時SP（高位に確保済み） */
    ldr   x0, =__stack_top
    mov   sp, x0

    /* 現在EL取得 */
    mrs   x0, CurrentEL
    lsr   x0, x0, #2          // x0 = 1,2,3

    /* EL3 -> EL2 に降格（Non-secure, AArch64）*/
    cmp   x0, #3
    b.ne  1f
    /* SCR_EL3: NS=1, RW=1 */
    mov   x1, #(1<<0)         // NS
    orr   x1, x1, #(1<<10)    // RW
    msr   SCR_EL3, x1
    /* SPSR_EL3: EL2h へ ERET (割り込みはマスク状態で降格) */
    mov   x1, #0x3C9          // EL=2h, mask D/A/I/F
    msr   SPSR_EL3, x1
    ldr   x1, =_enter_el2
    msr   ELR_EL3, x1
    eret

1:  /* EL2 -> EL1 へ降格、ついでにCNTVアクセス許可 */
    cmp   x0, #2
    b.ne  2f
_enter_el2:
    /* HCR_EL2: RW=1 (EL1 AArch64) */
    mov   x1, #(1<<31)
    msr   HCR_EL2, x1
    /* CNTHCTL_EL2: EL1PCTEN=1, EL1PCEN=1 → EL1から物理カウンタ/タイマ可
       さらに仮想タイマ用に CNTVOFF_EL2=0 （デフォルト0想定だが明示） */
    mov   x1, #3
    msr   CNTHCTL_EL2, x1
    msr   CNTVOFF_EL2, xzr
    isb

    /* SPSR_EL2: EL1h に戻す (割り込みはマスク状態のまま) */
    mov   x1, #0x3C5          // EL=1h, mask D/A/I/F
    msr   SPSR_EL2, x1
    ldr   x1, =_enter_el1
    msr   ELR_EL2, x1
    eret

2:
_enter_el1:
    /* VBAR_EL1 設定 */
    ldr   x1, =vector_table_el1
    msr   VBAR_EL1, x1
    isb

    /* EL1 では SP_EL1 を使用 */
    mov   x0, #1
    msr   SPSel, x0
    ldr   x0, =__stack_top
    mov   sp, x0
    bl    main

halt:
    wfi
    b halt
