/* locore.s
 * Assembler startup file for the STM32F103
 * Tom Trebisky  9-24-2016
 */

.word   0x10008000  /* stack top address */
.word   _reset      /* 1 Reset */
.word   hang        /* 2 NMI */
.word   hang        /* 3 Hard Fault */
.word   hang        /* 4 MM Fault */
.word   hang        /* 5 Bus Fault */
.word   hang        /* 6 Usage Fault */
.word   hang        /* 7 RESERVED */
.word   hang        /* 8 RESERVED */
.word   hang        /* 9 RESERVED*/
.word   hang        /* 10 RESERVED */
.word   hang        /* 11 SV call */
.word   hang        /* 12 Debug reserved */
.word   hang        /* 13 RESERVED */
.word   hang        /* 14 PendSV */
.word   hang        /* 15 SysTick */
.word   hang        /* 16 IRQ0 */
.word   hang        /* 17 IRQ1 */
.word   hang        /* 18 IRQ2 */
.word   hang        /* 19 ...   */
		    /* On to IRQ67 */

hang:   b .

.globl _reset
_reset:
    bl startup

/* THE END */
