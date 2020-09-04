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
	volatile unsigned long ccmr[2];	/* 18 */
	volatile unsigned long ccer;	/* 20 */
	volatile unsigned long cnt;	/* 24 */
	volatile unsigned long psc;	/* 28 */
	volatile unsigned long arr;	/* 2c */
	    long	_pad0;
	volatile unsigned long ccr[4];	/* 34 */
	    long	_pad1;
	volatile unsigned long dcr;	/* 48 */
	volatile unsigned long dmar;	/* 4c */
};

#define TIMER1_BASE	(struct timer *) 0x40012C00
#define TIMER2_BASE	(struct timer *) 0x40000000
#define TIMER3_BASE	(struct timer *) 0x40000400
#define TIMER4_BASE	(struct timer *) 0x40000800

#define	CR1_ENABLE	1	/* enable the counter */
#define ONE_PULSE	0x08	/* no autoreload */
#define DIR_DOWN	0x10	/* count down (only valid in certain modes) */

#define	UPDATE_IE	1	/* enable update interrupts */

#define	EGR_CC1		2
#define	EGR_CC2		4
#define	EGR_CC3		8
#define	EGR_CC4		0x10

#define CHAN1		0
#define CHAN2		1
#define CHAN3		2
#define CHAN4		3

#define TOGGLE		0x30	/* for ccmr register */

/* --------- */

#define	TIMER2_IRQ	28

/* On page 92 of the reference manual is the all important clock diagram.
 * It shows that Timer 1 gets the raw PCLK2 (72 Mhz) unscaled.
 * However Timers 2,3,4 get PKCL1 * 2 (36*2) = 72 Mhz.
 * So all the timers get a 72 Mhz clock the way I set things up.
 * And experiment bears this out for Timer 2.
 */

/* AN4776 is the 73 page STM32 Timer "cookbook" and seems like an
 * excellent document for understanding this timer.
 *
 * Note that an upcounting timer, counts from 0 to the ARR value.
 * A downcounting timer counts from the ARR value to 0.
 */

enum ttest { TEST1, TEST2, TEST3 };

static enum ttest cur_test = TEST1;

static int talk;
static int tcount;

static int led_state = 0;

/* Interrupt handler */
void
tim2_handler ( void )
{
	struct timer *tp = TIMER2_BASE;

	tp->sr = 0;	/* cancel the interrupt */

	if ( cur_test == TEST3 ) {
	    if ( led_state ) {
		led_state = 0;
		led_on ();
	    } else {
		led_state = 1;
		led_off ();
	    }
	} else {
	    /* TEST1 */
	    if ( talk == 0 )
		return;

	    serial_putc ( '0' + tcount );
	    // serial_putc ( 'A' + tcount );
	    if ( ++tcount > 50 ) {
		// tp->cr1 = 0;
		// tp->dier = 0;
		serial_puts ( " DONE" );
		talk = 0;
	    }
	}
}

int
timer_get ( void )
{
	struct timer *tp = TIMER2_BASE;

	return tp->cnt;
}

static void
timer_cc ( int chan, int load )
{
	struct timer *tp = TIMER2_BASE;
	int val;
	int mr = chan / 2;

	tp->ccr[chan] = load;

	/* Enable the output, active high */
	val = tp->ccer;
	val &= ~(0xf<<(chan*4));
	tp->ccer = val | (1<<(chan*4));

	/* Toggle output on compare match */
	val = tp->ccmr[mr];
	val &= ~(0xff<<((chan%2)*8));
	val |= TOGGLE<<((chan%2)*8);
	tp->ccmr[mr] = val;
}

static void
test1 ( void )
{
	struct timer *tp = TIMER2_BASE;

	tcount = 0;
	talk = 1;
	tp->arr = 2000;		/* 0x7d0 */
	tp->psc = 5000;

	/* This does one time event simulation */
	// tp->egr = EGR_CC1;

	nvic_enable ( TIMER2_IRQ );
	tp->dier = UPDATE_IE;

	tp->cr1 = CR1_ENABLE;
}

static void
test2 ( void )
{
	struct timer *tp = TIMER2_BASE;

	/* Enable A15 at Timer 2, channel 1 output */
	gpio_timer ();

	tp->arr = 3;
	tp->psc = 36;
	/* The above prescaler takes 72 Mhz down to 2 Mhz */
	/* The timer then divides by 4 = 500 khz,
	 * but this yields the toggle signal, so we should
	 * see 250 kHz, we actually see 243 kHz.
	 * This is off by 3 percent ....
	 */

	/* Timer 2, pin 1 is A15 of 0-15 */
	timer_cc ( CHAN1, 2 );
	timer_cc ( CHAN2, 2 );
	timer_cc ( CHAN3, 2 );
	timer_cc ( CHAN4, 2 );

	/*
	nvic_enable ( TIMER2_IRQ );
	tp->dier = UPDATE_IE;
	*/

	tp->cr1 = CR1_ENABLE;
}

/* This is the old test1, but we set things
 * up to blink the on-board LED
 * 
 * PSC is the prescaler.
 * ARR is the auto reload register.
 * note that this is a 16 bit timer, so max load is 65535
 *
 * Indeed, this works fine and blinks the LED at a nice rate.
 *  9-4-2020  This confirms that some peripheral on APB1 is working.
 */
static void
test3 ( void )
{
	struct timer *tp = TIMER2_BASE;

	cur_test = TEST3;

	tp->arr = 2000;		/* 0x7d0 */
	tp->psc = 5000;

	nvic_enable ( TIMER2_IRQ );
	tp->dier = UPDATE_IE;

	tp->cr1 = CR1_ENABLE;
}

void
timer_init ( void )
{
	// test1 ();
	// test2 ();
	test3 ();
}

/* THE END */
