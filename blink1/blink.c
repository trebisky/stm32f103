/* blink.c */

struct gpio {
	unsigned long cr[2];
	unsigned long idr;
	unsigned long odr;
	unsigned long bsrr;
	unsigned long brr;
	unsigned long lock;
};

#define GPIOA_BASE	(struct gpio *) 0x40010800
#define GPIOB_BASE	(struct gpio *) 0x40010C00
#define GPIOC_BASE	(struct gpio *) 0x40011000

#define MODE_OUT_2	0x02	/* Output, 2 Mhz */

#define CONF_GP_UD	0x0	/* Pull up/down */
#define CONF_GP_OD	0x4	/* Open drain */

void
delay ( void )
{
	volatile int count = 1000 * 1000;

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

	gp = GPIOC_BASE;

	shift = (bit - 8) * 4;
	conf = gp->cr[1] & ~(0xf<<shift);
	conf |= (MODE_OUT_2|CONF_GP_OD) << shift;
	gp->cr[1] = conf;

	on_mask = 1 << bit;
	off_mask = 1 << (bit+16);
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
