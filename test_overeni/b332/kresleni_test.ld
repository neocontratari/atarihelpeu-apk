/* B332: rozvrzeni programu kresleni_test pro PlayStation - vse od 0x80010000 za sebou */
ENTRY(_start)
SECTIONS
{
  . = 0x80010000;
  .text : { *(.text.start) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data .data.* .sdata .sdata.*) }
  .bss : { *(.bss .bss.* .sbss .sbss.* COMMON) }
  /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.pdr) *(.comment) *(.mdebug.*) *(.gnu.attributes) }
}
