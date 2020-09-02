/* i2c.c
 * (c) Tom Trebisky  8-31-2020
 *
 * Driver for the STM32F103 i2c
 *
 * Chapter 10 of the Tech manual describes the interrupt
 *	vector table.
 * Chapter 7 (7.3.8) gives the "enable" bits in the RCC
 *	to gate the clock to the i2c units
 *	(see rcc.c)
 * Chapter 26 is all about the i2c (page 755)
 *
 * And don't forget the clock diagram on page 92
 */

/* One of the 2 i2c units */

struct i2c {
	volatile unsigned long cr1;	/* 00 - control reg 1 */
	volatile unsigned long cr2;	/* 04 - control reg 2 */
	volatile unsigned long oar1;	/* 08 - own addr reg 1 */
	volatile unsigned long oar2;	/* 0c - own addr reg 2 */
	volatile unsigned long dr;	/* 10 - data register */
	volatile unsigned long sr1;	/* 14 - status reg 1 */
	volatile unsigned long sr2;	/* 18 - status reg 1 */
	volatile unsigned long ccr;	/* 1c - clock control reg */
	volatile unsigned long trise;	/* 20 - rise time reg */
};

#define I2C1_BASE	(struct i2c *) 0x40005400
#define I2C2_BASE	(struct i2c *) 0x40005800
#define NUM_I2C		2

struct i2c_stuff {
	struct i2c *base;
};

static struct i2c_stuff i2c_softc[NUM_I2C] = { I2C1_BASE, I2C2_BASE };

#define CR1_ENABLE		1

#define CCR_MODE_FAST		0x8000
#define CCR_MODE_DUTY_169	0x4000

#define	I2C1_EV_IRQ	31
#define	I2C1_ER_IRQ	32
#define	I2C2_EV_IRQ	33
#define	I2C2_ER_IRQ	34

#define TEST_DEVICE	0

/* --------- */


/* Interrupt handlers */
void i2c1_ev_handler ( void ) { }
void i2c1_er_handler ( void ) { }
void i2c2_ev_handler ( void ) { }
void i2c2_er_handler ( void ) { }

/* In general we run the STM32 at 72 Mhz and set
 * pclk1 to 36 Mhz, but we fetch the rate in Hz
 */
static void
init_i2c_clocks ( int device, int fast )
{
	struct i2c *icp;
	int pclk;

	icp = i2c_softc[device].base;

	icp->cr1 &= ~CR1_ENABLE;

	pclk = get_pclk1 ();

	/* slow (100 kHz) */
	/* fast (400 kHz) */
}

static void
test_i2c ( int device )
{
	nvic_enable ( I2C1_EV_IRQ );
	nvic_enable ( I2C1_ER_IRQ );

	nvic_enable ( I2C2_EV_IRQ );
	nvic_enable ( I2C2_ER_IRQ );

	init_i2c_clocks ( device, 0 );
}

void
i2c_init ( void )
{
	test_i2c ( TEST_DEVICE );
}

/* THE END */
