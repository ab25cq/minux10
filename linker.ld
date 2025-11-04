ENTRY(_start)

SECTIONS {
  . = 0x80000;       /* Piの64bitカーネル既定ロードアドレス */

  .text : { *(.text.boot) *(.text*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }

  . = ALIGN(16);
  __stack_top = . + 0x4000;  /* 16KB スタック */
}

