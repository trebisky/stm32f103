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
.word   bogus        /* 2 NMI */
.word   bogus        /* 3 Hard Fault */
.word   bogus        /* 4 MM Fault */
.word   bogus        /* 5 Bus Fault */
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
.word	bogus		/* IRQ  0 */
.word	bogus		/* IRQ  1 */
.word	bogus		/* IRQ  2 */
.word	bogus		/* IRQ  3 -- RTC */
.word	bogus		/* IRQ  4 */
.word	bogus		/* IRQ  5 */
.word	bogus		/* IRQ  6 */
.word	bogus		/* IRQ  7 */
.word	bogus		/* IRQ  8 */
.word	bogus		/* IRQ  9 */
.word	bogus		/* IRQ 10 */
.word	bogus		/* IRQ 11 */
.word	bogus		/* IRQ 12 */
.word	bogus		/* IRQ 13 */
.word	bogus		/* IRQ 14 */
.word	bogus		/* IRQ 15 */
.word	bogus		/* IRQ 16 */
.word	bogus		/* IRQ 17 */
.word	bogus		/* IRQ 18 */
.word	usb_hp_handler	/* IRQ 19 */
.word	usb_lp_handler	/* IRQ 20 */
.word	bogus		/* IRQ 21 */
.word	bogus		/* IRQ 22 */
.word	bogus		/* IRQ 23 */
.word	bogus		/* IRQ 24 -- Timer 1 break */
.word	bogus		/* IRQ 25 -- Timer 1 update */
.word	bogus		/* IRQ 26 -- Timer 1 trig */
.word	bogus		/* IRQ 27 -- Timer 1 cc */
.word	tim2_handler	/* IRQ 28 -- Timer 2 */
.word	bogus		/* IRQ 29 -- Timer 3 */
.word	bogus		/* IRQ 30 -- Timer 4 */
.word	bogus		/* IRQ 31 */
.word	bogus		/* IRQ 32 */
.word	bogus		/* IRQ 33 */
.word	bogus		/* IRQ 34 */
.word	bogus		/* IRQ 35 */
.word	bogus		/* IRQ 36 */
.word	bogus		/* IRQ 37 -- UART 1 */
.word	bogus		/* IRQ 38 -- UART 2 */
.word	bogus		/* IRQ 39 -- UART 3 */
.word	bogus		/* IRQ 40 */
.word	bogus		/* IRQ 41 */
.word	usb_wk_handler	/* IRQ 42 */
.word	bogus		/* IRQ 43 */
.word	bogus		/* IRQ 44 */
.word	bogus		/* IRQ 45 */
.word	bogus		/* IRQ 46 */
.word	bogus		/* IRQ 47 */
.word	bogus		/* IRQ 48 */
.word	bogus		/* IRQ 49 */
.word	bogus		/* IRQ 50 */
.word	bogus		/* IRQ 51 */
.word	bogus		/* IRQ 52 */
.word	bogus		/* IRQ 53 */
.word	bogus		/* IRQ 54 */
.word	bogus		/* IRQ 55 */
.word	bogus		/* IRQ 56 */
.word	bogus		/* IRQ 57 */
.word	bogus		/* IRQ 58 */
.word	bogus		/* IRQ 59 */

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

.thumb_func
bogus:   b .

.thumb_func
_reset:
    bl startup
    b .

/* THE END */
