/* gpio.c
 * (c) Tom Trebisky  7-2-2017, 9-12-2017
 *
 * This is documented in section 9 of the reference manual.
 */

/* One of the 3 gpios */
struct gpio {
	// volatile unsigned long cr[2];
	volatile unsigned long crl;
	volatile unsigned long crh;
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

/* Change these two to select the output pin/port */
#ifdef notdef
#define BLINK_GPIO	GPIOC_BASE
#define BLINK_BIT	13
#define BLINK_GPIO	GPIOB_BASE
#define BLINK_BIT	4
#endif

#define BLINK_GPIO	GPIOB_BASE
#define BLINK_BIT	4

struct gpio *gp;
unsigned long on_mask;
unsigned long off_mask;

void
led_init ( void )
{
	int conf;
	int shift;
	int bit = BLINK_BIT;

	gp = BLINK_GPIO;

	if ( bit < 8 ) {
	    shift = bit * 4;
	    conf = gp->crl & ~(0xf<<shift);
	    conf |= (MODE_OUT_2|CONF_GP_OD) << shift;
	    gp->crl = conf;
	} else {
	    shift = (bit - 8) * 4;
	    conf = gp->crh & ~(0xf<<shift);
	    conf |= (MODE_OUT_2|CONF_GP_OD) << shift;
	    gp->crh = conf;
	}

	off_mask = 1 << bit;
	on_mask = 1 << (bit+16);
}

void
led_on ( void )
{
	gp->bsrr = on_mask;
}

void
led_off ( void )
{
	gp->bsrr = off_mask;
}

/* This yields a 1 Mhz waveform */
void
led_fast ( void )
{
	for ( ;; ) {
	    gp->bsrr = on_mask;
	    gp->bsrr = on_mask;
	    gp->bsrr = off_mask;
	}
}

/* THE END */
