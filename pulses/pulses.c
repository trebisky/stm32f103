/* pulses.c
 * (c) Tom Trebisky  9-24-2016 10-16-2026
 */

/* The reset and clock control module */
struct rcc {
	volatile unsigned long rc;	/* 0 - clock control */
	volatile unsigned long cfg;	/* 4 - clock config */
	volatile unsigned long cir;	/* 8 - clock interrupt */
	volatile unsigned long apb2;	/* c - peripheral reset */
	volatile unsigned long apb1;	/* 10 - peripheral reset */
	volatile unsigned long ape3;	/* 14 - peripheral enable */
	volatile unsigned long ape2;	/* 18 - peripheral enable */
	volatile unsigned long ape1;	/* 1c - peripheral enable */
	volatile unsigned long bdcr;	/* 20 - xx */
	volatile unsigned long csr;	/* 24 - xx */
};

#define GPIOA_ENABLE	0x04
#define GPIOB_ENABLE	0x08
#define GPIOC_ENABLE	0x10

#define RCC_BASE	(struct rcc *) 0x40021000

/* One of the 3 gpios */
struct gpio {
	volatile unsigned long cr[2];
	volatile unsigned long idr;
	volatile unsigned long odr;
	volatile unsigned long bsrr;
	volatile unsigned long brr;
	volatile unsigned long lock;
};

#define GPIOA_BASE	(struct gpio *) 0x40010800
#define GPIOB_BASE	(struct gpio *) 0x40010C00
#define GPIOC_BASE	(struct gpio *) 0x40011000

#define MODE_OUT_2	0x02	/* Output, 2 Mhz */

#define CONF_GP_UD	0x0	/* Pull up/down */
#define CONF_GP_OD	0x4	/* Open drain */

/* This gives a blink rate of about 2.7 Hz */
/* My scope says 2.87 Hz */
/* i.e. the delay time is about 0.2 seconds (200 ms) */
#define FAST	200

#define FASTER	50

/* This gives a 1.02 Hz square wave according to my scope */
#define SLOWER	555

void
delay ( void )
{
	// volatile int count = 1000 * FAST;
	volatile int count = 1000 * SLOWER;

	while ( count-- )
	    ;
}

struct gpio *gpa;
struct gpio *gpc;

unsigned long on_mask;
unsigned long off_mask;

unsigned long aon_mask;
unsigned long aoff_mask;

/* Each gpio controls 16 bits.
 * The two cr[] registers each configure
 * 8 bits.
 */

void
gpio_init ( void )
{
	int conf;
	int shift;
	struct rcc *rp = RCC_BASE;
	int bit;

	/* Turn on GPIO A, B and C */
	rp->ape2 |= GPIOA_ENABLE;
	rp->ape2 |= GPIOB_ENABLE;
	rp->ape2 |= GPIOC_ENABLE;

// First the LED
// open drain works well here
#define PC13	13

	bit = PC13;
	gpc = GPIOC_BASE;

	shift = (bit - 8) * 4;
	conf = gpc->cr[1] & ~(0xf<<shift);
	conf |= (MODE_OUT_2|CONF_GP_OD) << shift;
	gpc->cr[1] = conf;
	off_mask = 1 << bit;
	on_mask = 1 << (bit+16);

// Next the gpio pin for pulses (A0)
// here we need pull up/down

	bit = 0;
	gpa = GPIOA_BASE;

	shift = (bit) * 4;
	conf = gpa->cr[0] & ~(0xf<<shift);
	conf |= (MODE_OUT_2|CONF_GP_UD) << shift;
	gpa->cr[0] = conf;

	aoff_mask = 1 << bit;
	aon_mask = 1 << (bit+16);
}

void
led_on ( void )
{
	gpc->bsrr = on_mask;
}

void
led_off ( void )
{
	gpc->bsrr = off_mask;
}

void
pin_on ( void )
{
	gpa->bsrr = aon_mask;
}

void
pin_off ( void )
{
	gpa->bsrr = aoff_mask;
}

// #define TWICE

void
startup ( void )
{
	gpio_init ();

	for ( ;; ) {
	    led_on ();
		pin_on ();
	    delay ();
	    led_off ();
		pin_off ();
	    delay ();

#ifdef TWICE
	    led_on ();
	    delay ();
	    led_off ();
	    delay ();
	    delay ();
	    delay ();
#endif
	}
}

/* THE END */
