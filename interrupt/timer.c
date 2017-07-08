/* Timer.c
 * (c) Tom Trebisky  7-5-2017
 *
 * Driver for the STM32F103 timer
 */

/* One of the 4 timers */

/* Note that timer 1 is the "advanced" timer and this
 * code is written for the general purpose timers 2,3,4
 */

/* Timer 1 gets a version of PCLK2
 * Timer 2,3,4 get a version of PCLK1
 * If the PCLK prescaler is == 1, they get the straight PCLK
 * if the PCLK prescaler is != 1, they get PCLK * 2
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

#define	CR1_ENABLE	1	/* enable the counter */
#define	UPDATE_IE	1	/* enable update interrupts */

/* ---------------------------- */
/* ---------------------------- */

struct nvic {
	volatile unsigned long iser0;	/* 00 */
	volatile unsigned long iser1;	/* 04 */
	volatile unsigned long iser2;	/* 08 */
	volatile unsigned long icer0;	/* 0c */
	volatile unsigned long icer1;	/* 10 */
	volatile unsigned long icer2;	/* 14 */
	/* ... */
};

#define NVIC_BASE   ((struct nvic *) 0xe000e100)

/* ---------------------------- */
/* ---------------------------- */

static int tcount;

/* Interrupt handler */
void
tim2_handler ( void )
{
	struct timer *tp = TIMER2_BASE;

	tp->sr = 0;	/* cancel the interrupt */

	serial_putc ( 'A' + tcount );
	if ( ++tcount > 32 ) {
	    tp->cr1 = 0;
	    tp->dier = 0;
	    serial_putc ( '@' );
	    serial_putc ( '@' );
	}
}

int
timer_get ( void )
{
	struct timer *tp = TIMER2_BASE;

	return tp->cnt;
}

/* Timer 2, pin 1 is A15 of 0-15 */
void
timer_init ( void )
{
	struct timer *tp = TIMER2_BASE;
	struct nvic *np = NVIC_BASE;

	gpio_timer ();

	tcount = 0;
	tp->arr = 20000;
	tp->psc = 1000;
	tp->cr1 = CR1_ENABLE;
	tp->dier = UPDATE_IE;
	// np->iser0 = 0x02000000;	/* timer 1 upd - irq25 */
	np->iser0 = 0x10000000;		/* timer 2 - irq28 */
}

/* THE END */
