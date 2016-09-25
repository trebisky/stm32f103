/* blink.c
 * (c) Tom Trebisky  9-24-2016
 *
 * This is just a minor tweak on the first
 * demo to try out some code and compiler
 * optimization.
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

/* This file is an experiment with using these types
 * of constructions.  They help a little bit to generate
 * better code, but frankly with the -O2 switch,
 * the compiler does a pretty good job.
 * Making the led_on and off functions macros
 * or just moving the code inline would be even
 * better.  Not that it matters here (we are
 * burning CPU cycles in the delay loop anyway),
 * but I am interesting in looking at the objdump
 * output and learning proper tricks for places
 * where time (and more importantly space) matter.
 */
#define RCC_p		(RCC_BASE)
#define GPIOA_p		(GPIOA_BASE)
#define GPIOB_p		(GPIOB_BASE)
#define GPIOC_p		(GPIOC_BASE)

#define MODE_OUT_2	0x02	/* Output, 2 Mhz */

#define CONF_GP_UD	0x0	/* Pull up/down */
#define CONF_GP_OD	0x4	/* Open drain */

/* This gives a blink rate of about 2.7 Hz */
void
delay ( void )
{
	volatile int count = 1000 * 200;

	while ( count-- )
	    ;
}

struct gpio *gp;
unsigned long on_mask;
unsigned long off_mask;

void
led_init ( int bit )
{
	int conf;
	int shift;
	// struct rcc *rp = RCC_BASE;

	/* Turn on GPIO C */
	RCC_p->ape2 |= GPIOC_ENABLE;

	// gp = GPIOC_BASE;

	shift = (bit - 8) * 4;
	conf = GPIOC_p->cr[1] & ~(0xf<<shift);
	conf |= (MODE_OUT_2|CONF_GP_OD) << shift;
	GPIOC_p->cr[1] = conf;

	on_mask = 1 << bit;
	off_mask = 1 << (bit+16);
}

void
led_on ( void )
{
	GPIOC_p->bsrr = on_mask;
}

void
led_off ( void )
{
	GPIOC_p->bsrr = off_mask;
}

#define PC13	13

void
startup ( void )
{
	led_init ( PC13 );

	for ( ;; ) {
	    led_on ();
	    delay ();
	    led_off ();
	    delay ();
	}
}

/* THE END */
