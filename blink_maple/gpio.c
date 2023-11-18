/* gpio.c
 * (c) Tom Trebisky  7-2-2017
 */

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

#define CONF_GP_PP	0x0	/* Push pull */
#define CONF_GP_OD	0x4	/* Open drain */

struct gpio *gp;
unsigned long on_mask;
unsigned long off_mask;

/* XXX - GPIO is "wired" in */

#define MAPLE

void
led_init ( void )
{
	int conf;
	int shift;
	int bit;

#ifdef MAPLE
	/* A5 for Maple */
	bit = 5;
	shift = bit * 4;
	gp = GPIOA_BASE;
	conf = gp->cr[0] & ~(0xf<<shift);
	conf |= (MODE_OUT_2|CONF_GP_PP) << shift;
	gp->cr[0] = conf;
#else
	/* C13 for Pill */
	bit = 13;
	shift = (bit - 8) * 4;
	gp = GPIOC_BASE;
	conf = gp->cr[1] & ~(0xf<<shift);
	conf |= (MODE_OUT_2|CONF_GP_OD) << shift;
	gp->cr[1] = conf;
#endif

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

/* THE END */
