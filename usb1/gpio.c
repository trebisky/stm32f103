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

/* Each gpio has 16 bits.
 * The CR registers each control 8 of these bits.
 * cr[0] == cr_low  configures 0-7
 * cr[1] == cr_high configures 8-15
 * Each bit is controlled by a 4 bit field
 * The reset state is 0x4 for all (input, floating)
 */

/* there are only 3 choices for inputs */
#define INPUT_ANALOG	0x0
#define INPUT_FLOAT	0x4
#define INPUT_PUPD	0x8

/* For outputs, combine one from the following list of 3
 * with one of the 4 that follow.
 */
#define OUTPUT_10M	1
#define OUTPUT_2M	2
#define OUTPUT_50M	3

#define OUTPUT_PUSH_PULL	0
#define OUTPUT_ODRAIN		4

#define ALT_PUSH_PULL		8
#define ALT_ODRAIN		0xc

struct gpio *led_gp;
unsigned long on_mask;
unsigned long off_mask;

static void
gpio_mode ( struct gpio *gp, int bit, int mode )
{
	int reg;
	int conf;
	int shift;

	reg = bit / 8;
	shift = (bit%8) * 4;

	conf = gp->cr[reg] & ~(0xf<<shift);
	gp->cr[reg] = conf | (mode << shift);
}

void
gpio_a_set ( int bit, int val )
{
	struct gpio *gp = GPIOA_BASE;

	if ( val )
	    gp->bsrr = 1 << bit;
	else
	    gp->bsrr = 1 << (bit+16);
}

void
gpio_b_set ( int bit, int val )
{
	struct gpio *gp = GPIOB_BASE;

	if ( val )
	    gp->bsrr = 1 << bit;
	else
	    gp->bsrr = 1 << (bit+16);
}

void
gpio_c_set ( int bit, int val )
{
	struct gpio *gp = GPIOC_BASE;

	if ( val )
	    gp->bsrr = 1 << bit;
	else
	    gp->bsrr = 1 << (bit+16);
}

void
gpio_a_init ( int bit )
{
	gpio_mode ( GPIOA_BASE, bit, OUTPUT_50M | OUTPUT_PUSH_PULL );
}

void
led_init ( int bit )
{
	gpio_mode ( GPIOC_BASE, bit, OUTPUT_2M | OUTPUT_ODRAIN );

	led_gp = GPIOC_BASE;
	off_mask = 1 << bit;
	on_mask = 1 << (bit+16);
}

void
led_on ( void )
{
	led_gp->bsrr = on_mask;
}

void
led_off ( void )
{
	led_gp->bsrr = off_mask;
}

void
gpio_uart1 ( void )
{
	gpio_mode ( GPIOA_BASE, 9, OUTPUT_50M | ALT_PUSH_PULL );
	// gpio_mode ( GPIOA_BASE, 10, INPUT_FLOAT );
}

void
gpio_uart2 ( void )
{
	gpio_mode ( GPIOA_BASE, 2, OUTPUT_50M | ALT_PUSH_PULL );
	// gpio_mode ( GPIOA_BASE, 3, INPUT_FLOAT );
}

void
gpio_uart3 ( void )
{
	gpio_mode ( GPIOB_BASE, 10, OUTPUT_50M | ALT_PUSH_PULL );
	// gpio_mode ( GPIOB_BASE, 11, INPUT_FLOAT );
}

/* XXX - this is one special case, we have 16 to consider */
/* Timer 2, pin 1 is A15 of 0-15 */
void
gpio_timer ( void )
{
	/* Timer 2, outputs  1,2,3,4 */
	gpio_mode ( GPIOA_BASE, 15, OUTPUT_50M | ALT_PUSH_PULL );
	gpio_mode ( GPIOA_BASE, 1, OUTPUT_50M | ALT_PUSH_PULL );
	gpio_mode ( GPIOA_BASE, 2, OUTPUT_50M | ALT_PUSH_PULL );
	gpio_mode ( GPIOA_BASE, 3, OUTPUT_50M | ALT_PUSH_PULL );
}

/* THE END */
