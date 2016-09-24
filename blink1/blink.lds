/* blink.lds
 * linker script for blink demo.
 */
MEMORY
{
   flash(RX)  : ORIGIN = 0x00000000, LENGTH = 0x40000
   sram(WAIL) : ORIGIN = 0x10000000, LENGTH = 30K
}

SECTIONS
{
   .text : { *(.text*) } > flash
   .bss  : { *(.bss*) } > sram
}
