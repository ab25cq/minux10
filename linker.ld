ENTRY(_start)

SECTIONS {
  . = 0x80000;          /* Raspberry Pi の標準ブートアドレス */

  .text : {
    *(.text.boot)
    *(.text*)
  }

  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : {
    *(.bss*)
    *(COMMON)
  }
}

