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
 * Chapter 9 talks about AFIO (alternate function IO) pins
 * Chapter 26 is all about the i2c (page 755)
 *
 * And don't forget the clock diagram on page 92
 *
 * i2c1 can appear in two places
 * B8 and B9 -- B8 = SCL,  B9 = SDA
 * B6 and B7 -- B6 = SCL,  B7 = SDA
 * i2c2 can appear only in one place.
 * B10 = SCL,  B11 = SDA
 *
 * Linux has an i2c driver for an stm32f4 chip that looks applicable:
 * cd /u1/Projects/Femto/Openwrt/build_dir/target-mipsel_24kc_musl/linux-ramips_rt305x/linux-4.14.180
 * cd drivers/i2c/busses
 * vi i2c-stm32f4.c
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

#define CR1_START		0x0100
#define CR1_STOP		0x0200
#define CR1_ACK_ENA		0x0400
#define CR1_ENABLE		1

#define CR2_KEEP		0xe0c0
#define CR2_BUF_INT_ENA		0x0400
#define CR2_EV_INT_ENA		0x0200
#define CR2_ER_INT_ENA		0x0100
#define CR2_ALL_INT_ENA		0x0700

#define CCR_MODE_FAST		0x8000
#define CCR_MODE_DUTY_169	0x4000
#define CCR_KEEP		0x3000

#define	I2C1_EV_IRQ	31
#define	I2C1_ER_IRQ	32
#define	I2C2_EV_IRQ	33
#define	I2C2_ER_IRQ	34

/* --------- */

static void i2c_showr ( struct i2c * );


/* Interrupt handlers */
void
i2c1_ev_handler ( void )
{
	serial_puts ( "i2c 1 event\n" );
	i2c_showr ( i2c_softc[0].base );
}

void
i2c1_er_handler ( void )
{
	serial_puts ( "i2c 1 error\n" );
	i2c_showr ( i2c_softc[0].base );
}

void i2c2_ev_handler ( void )
{
	serial_puts ( "i2c 2 event\n" );
	i2c_showr ( i2c_softc[1].base );
}

void i2c2_er_handler ( void )
{
	serial_puts ( "i2c 2 error\n" );
	i2c_showr ( i2c_softc[1].base );
}

/* In general we run the STM32 at 72 Mhz and set
 * pclk1 to 36 Mhz, but we fetch the rate in Hz
 *  (but we never use it)
 * At 36 Mhz, the pclk has a 27.777 ns period
 *  So to get the slow factor, for the slow clock,
 *  Thi = Tlo = 5000 ns and ccr = 5000/27.777 = 180
 * The slow factor, we get two choices.
 *  With the duty bit set 0, we have Tlo = 2 * Thi
 *  and Tlo + Thi = 2500 ns.
 *  ccr = 2500 / 3 / 27.77
 * I was too lazy to check when the duty bit is set 1
 *  When it is, Thi = 9 * x, Tlo = 16 * x and
 *  x = ccr * Tpclk, where Tpclk = 27.777 ns.
 * As long as ccr fits into 12 bits we can use duty=0
 * We would have to have a very fast pclk to need to
 *  set duty=1 (or be very fussy about 400 kHz).
 */
static void
init_i2c_clocks ( int device, int fast )
{
	struct i2c *icp;
	// int pclk = get_pclk1 ();
	int ccr;

	icp = i2c_softc[device].base;

	ccr = icp->ccr & CCR_KEEP;
	if ( fast ) {
	    /* fast (400 kHz) */
	    ccr |= CCR_MODE_FAST;
	    ccr |= 30;
	    icp->ccr = ccr;
	    icp->trise = 11;
	} else {
	    /* slow (100 kHz) */
	    ccr |= 180;
	    icp->ccr = ccr;
	    icp->trise = 37;
	}
	show_reg ( "i2c clocks, ccr", &icp->ccr );
	show_reg ( "i2c clocks, trise", &icp->trise );
}

static void
i2c_start ( struct i2c *icp )
{
	int timo;
	int cr;

	serial_puts ( "Starting i2c\n" );
	show_reg ( "i2c, starting", &icp->cr1 );

	// icp->cr1 |= CR1_START;
	cr = icp->cr1;
	cr |= CR1_START;
	icp->cr1 = cr;

	timo = 50000;
	while ( timo-- ) {
	    if ( ! (icp->cr1 & CR1_START) )
		break;
	}
	show_n ( "Started i2c", timo );
	show_reg ( "i2c, started", &icp->cr1 );
}

static void
i2c_showr ( struct i2c *icp )
{
	show_reg ( "i2c cr1", &icp->cr1 );
	show_reg ( "i2c cr2", &icp->cr2 );

	show_reg ( "i2c sr1", &icp->sr1 );
	show_reg ( "i2c sr2", &icp->sr2 );
}

/* Some notes on maximum rise time.
 * For Sm mode (100 Khz) it is 1000 ns
 * For Fm mode (400 Khz) it is 300 ns
 * With a 36 Mhz pclk, the pclk period is
 *  1000/36 = 27.778 ns
 * So for Sm, we set Trise = 36 + 1 = 0x25
 */

static void
init_i2c_hw ( int device )
{
	int freq = get_pclk1 () / (1000*1000);
	struct i2c *icp;
	int cr;
	int fast = 0;

	icp = i2c_softc[device].base;

	  i2c_showr ( icp );
	  serial_puts ( "-- Reset\n" );
	i2c_reset ();
	  i2c_showr ( icp );

	// icp->cr1 &= ~CR1_ENABLE;

	// cr = icp->cr2 & CR2_KEEP;
	cr |= freq;
	icp->cr2 = cr;

	show16 ( "pclk: ", freq );

	init_i2c_clocks ( device, fast );

	icp->cr2 |= CR2_ALL_INT_ENA;

	// icp->cr1 |= CR1_ENABLE;
	icp->cr1 = CR1_ENABLE;

	  i2c_showr ( icp );
	i2c_start ( icp );
	  i2c_showr ( icp );
}

static void
test_i2c ( int device )
{
	nvic_enable ( I2C1_EV_IRQ );
	nvic_enable ( I2C1_ER_IRQ );

	nvic_enable ( I2C2_EV_IRQ );
	nvic_enable ( I2C2_ER_IRQ );

	show16 ( "Testing i2c: ", device );

	if ( device == 0 ) {
	    gpio_i2c1 ();
	    // gpio_i2c1_alt ();
	} else {
	    gpio_i2c2 ();
	}

	init_i2c_hw ( device );
}

/* -------------------------------------------- */

#define TEST_DEVICE	0
// #define TEST_DEVICE	1


void
i2c_init ( void )
{
	test_i2c ( TEST_DEVICE );
}

void
i2c_debug ( void )
{
	i2c_showr ( i2c_softc[TEST_DEVICE].base );
}

/* THE END */
