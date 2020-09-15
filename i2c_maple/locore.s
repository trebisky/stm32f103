/* locore.s
 * Assembler startup file for the STM32F103
 * Tom Trebisky  9-24-2016
 */

# The Cortex M3 is a thumb only processor
.cpu cortex-m3
.thumb

@ First the "standard" 16 entries for a cortex-m3
.word   0x20005000   /* stack top address */
.word   _reset       /* 1 Reset */

.word   bogus_nmi        /* 2 NMI */
.word   bogus_hf        /* 3 Hard Fault */
.word   bogus_mm        /* 4 MM Fault */
.word   bogus_bus        /* 5 Bus Fault */
.word   bogus        /* 6 Usage Fault */
.word   bogus        /* 7   RESERVED */
.word   bogus        /* 8   RESERVED */
.word   bogus        /* 9   RESERVED*/
.word   bogus        /* 10  RESERVED */
.word   bogus        /* 11 SV call */
.word   bogus        /* 12   Debug reserved */
.word   bogus        /* 13   RESERVED */
.word   bogus        /* 14 PendSV */
.word   systick_handler        /* 15 SysTick */

@ and now 68 IRQ vectors
.word	bogus_irq		/* IRQ  0 */
.word	bogus_irq		/* IRQ  1 */
.word	bogus_irq		/* IRQ  2 */
.word	bogus_irq		/* IRQ  3 -- RTC */
.word	bogus_irq		/* IRQ  4 */
.word	bogus_irq		/* IRQ  5 */
.word	bogus_irq		/* IRQ  6 */
.word	bogus_irq		/* IRQ  7 */
.word	bogus_irq		/* IRQ  8 */
.word	bogus_irq		/* IRQ  9 */
.word	bogus_irq		/* IRQ 10 */
.word	bogus_irq		/* IRQ 11 */
.word	bogus_irq		/* IRQ 12 */
.word	bogus_irq		/* IRQ 13 */
.word	bogus_irq		/* IRQ 14 */
.word	bogus_irq		/* IRQ 15 */
.word	bogus_irq		/* IRQ 16 */
.word	bogus_irq		/* IRQ 17 */
.word	bogus_irq		/* IRQ 18 */
.word	bogus_irq		/* IRQ 19 */
.word	bogus_irq		/* IRQ 20 */
.word	bogus_irq		/* IRQ 21 */
.word	bogus_irq		/* IRQ 22 */
.word	bogus_irq		/* IRQ 23 */
.word	bogus_irq		/* IRQ 24 -- Timer 1 break */
.word	bogus_irq		/* IRQ 25 -- Timer 1 update */
.word	bogus_irq		/* IRQ 26 -- Timer 1 trig */
.word	bogus_irq		/* IRQ 27 -- Timer 1 cc */
.word	tim2_handler		/* IRQ 28 -- Timer 2 */
.word	bogus_irq		/* IRQ 29 -- Timer 3 */
.word	bogus_irq		/* IRQ 30 -- Timer 4 */
.word	i2c1_ev_handler		/* IRQ 31 */
.word	i2c1_er_handler		/* IRQ 32 */
.word	i2c2_ev_handler		/* IRQ 33 */
.word	i2c2_er_handler		/* IRQ 34 */
.word	bogus_irq		/* IRQ 35 */
.word	bogus_irq		/* IRQ 36 */
.word	serial1_handler		/* IRQ 37 -- UART 1 */
.word	serial2_handler		/* IRQ 38 -- UART 2 */
.word	serial3_handler		/* IRQ 39 -- UART 3 */
.word	bogus_irq		/* IRQ 40 */
.word	bogus_irq		/* IRQ 41 */
.word	bogus_irq		/* IRQ 42 */
.word	bogus_irq		/* IRQ 43 */
.word	bogus_irq		/* IRQ 44 */
.word	bogus_irq		/* IRQ 45 */
.word	bogus_irq		/* IRQ 46 */
.word	bogus_irq		/* IRQ 47 */
.word	bogus_irq		/* IRQ 48 */
.word	bogus_irq		/* IRQ 49 */
.word	bogus_irq		/* IRQ 50 */
.word	bogus_irq		/* IRQ 51 */
.word	bogus_irq		/* IRQ 52 */
.word	bogus_irq		/* IRQ 53 */
.word	bogus_irq		/* IRQ 54 */
.word	bogus_irq		/* IRQ 55 */
.word	bogus_irq		/* IRQ 56 */
.word	bogus_irq		/* IRQ 57 */
.word	bogus_irq		/* IRQ 58 */
.word	bogus_irq		/* IRQ 59 */

#ifdef notdef
/* These last 8 aren't assigned on the STM32F103, so we can save a few
 * bytes of flash.
 */
.word	bogus		/* IRQ 60 */
.word	bogus		/* IRQ 61 */
.word	bogus		/* IRQ 62 */
.word	bogus		/* IRQ 63 */
.word	bogus		/* IRQ 64 */
.word	bogus		/* IRQ 65 */
.word	bogus		/* IRQ 66 */
.word	bogus		/* IRQ 67 */
#endif

/* Move this to nvic.c for now.
.thumb_func
bogus:   b .
*/

.thumb_func
_reset:
    bl startup
    b .

/* THE END */
