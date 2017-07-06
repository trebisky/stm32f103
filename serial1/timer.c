/* Timer.c
 * (c) Tom Trebisky  7-5-2017
 *
 * Driver for the STM32F103 timer
 */

/* One of the 4 timers */

/* Note that timer 1 is the "advanced" timer and this
 * code is written for the general purpose timers 2,3,4
 */

struct timer {
	volatile unsigned long cr1;	/* 00 */
	volatile unsigned long cr2;	/* 04 */
	volatile unsigned long smcr;	/* 08 */
	volatile unsigned long dier;	/* 0c */
	volatile unsigned long sr;	/* 10 */
	volatile unsigned long egr;	/* 14 */
	volatile unsigned long ccmr1;	/* 18 */
	volatile unsigned long ccmr2;	/* 1c */
	volatile unsigned long ccer;	/* 20 */
	volatile unsigned long cnt;	/* 24 */
	volatile unsigned long psc;	/* 28 */
	volatile unsigned long arr;	/* 2c */
	    long	_pad0;
	volatile unsigned long ccr1;	/* 34 */
	volatile unsigned long ccr2;	/* 38 */
	volatile unsigned long ccr3;	/* 3c */
	volatile unsigned long ccr4;	/* 40 */
	    long	_pad1;
	volatile unsigned long dcr;	/* 48 */
	volatile unsigned long dmar;	/* 4c */
};

#define TIMER1_BASE	(struct timer *) 0x40012C00
#define TIMER2_BASE	(struct timer *) 0x40000000
#define TIMER3_BASE	(struct timer *) 0x40000400
#define TIMER4_BASE	(struct timer *) 0x40000800

void
timer_init ( void )
{
}

/* THE END */
