ENTRY(_start)

SECTIONS {
  . = 0x80000;

  .vectors ALIGN(2048) : {
    KEEP(*(.vectors))
  }
  . = ALIGN(4);

  .text : {
    *(.text.boot)
    *(.text*)
  }

  .rodata : { *(.rodata*) }
  .data   : { *(.data*) }
  .bss : { *(.bss*) *(COMMON) }

  . = ALIGN(16);
  __stack_top = . + 0x4000;
}

