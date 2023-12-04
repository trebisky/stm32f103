/* baboon.lds
 * linker script for baboon demo.
 *
 * Memory layout on my STM32F103 is pretty much like this:
 *
 * 0x00000000 - 0x07ffffff - aliased to flash or sys memory depending on BOOT jumpers
 * 0x08000000 - 0x0800ffff - Flash (64K ... or 128K)
 * 0x1ffff000 - 0x1ffff7ff - Boot firmware in system memory
 * 0x1ffff800 - 0x1fffffff - option bytes
 * 0x20000000 - 0x20004fff - SRAM (20k)
 * 0x40000000 - 0x40023400 - peripherals
 *
 *  flash(RX)  : ORIGIN = 0x00000000, LENGTH = 128K
 *  flash(RX)  : ORIGIN = 0x08005000, LENGTH = 128K
 */

/* The important thing for a Maple board is that the flash
 * is linked to offset 5000 to avoid overwriting the USB
 * loader.
 */
MEMORY
{
   flash(RX)  : ORIGIN = 0x08000000, LENGTH = 128K
   sram(RW)   : ORIGIN = 0x20000000, LENGTH = 20K
}

/* An experiment 12-3-2023
 */

PERIPH_BASE = 0x40000000;
APB1PERIPH = PERIPH_BASE;
USB    = APB1PERIPH + 0x5C00;
USB_PMA = APB1PERIPH + 0x6000;

PMA_buf = USB_PMA;
PMA_btable = USB_PMA;

SECTIONS
{
   .text :
   {
       *(.text*)
       . = ALIGN(4);
       __text_end = .;
       _etext = .;
   } > flash

   .bss  :
   {
       . = ALIGN(4);
       __bss_start = .;
       *(.bss*)
       *(COMMON)
       . = ALIGN(4);
       __bss_end = .;
   } > sram

   .data :
   {
       . = ALIGN(4);
       __data_start = .;
       *(.data*)
       . = ALIGN(4);
       __data_end = .;
   } > sram AT> flash

   .rodata :
   {
       . = ALIGN(4);
       *(.rodata*)
       . = ALIGN(4);
   } > flash

}
