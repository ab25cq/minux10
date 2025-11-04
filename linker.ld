ENTRY(_start)

SECTIONS {
  . = 0x80000;

  .text : ALIGN(2048) {
    __vectors_start = .;      /* 2KB アラインの例外ベクタ */
    KEEP(*(.vectors))
    . = ALIGN(4);
    *(.text.boot)
    *(.text*)
  }

  .rodata : { *(.rodata*) }
  .data   : { *(.data*) }
  .bss : {
    *(.bss*)
    *(COMMON)
  }

  . = ALIGN(16);
  __stack_top = . + 0x4000;   /* 16KB スタック */
}
