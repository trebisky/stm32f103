/* i2c.c
 * (c) Tom Trebisky  8-31-2020
 *
 * Driver for the STM32F103 i2c
 */

/* One of the 2 i2c units */

struct i2c {
	volatile unsigned long cr1;	/* 00 */
	volatile unsigned long cr2;	/* 04 */
};

#define I2C1_BASE	(struct timer *) 0x40012C00
#define I2C2_BASE	(struct timer *) 0x40000000

/* --------- */

#define	I2C1_EV_IRQ	31
#define	I2C1_ER_IRQ	32
#define	I2C2_EV_IRQ	33
#define	I2C2_ER_IRQ	34

/* On page 92 of the reference manual is the all important clock diagram.
 * It shows that Timer 1 gets the raw PCLK2 (72 Mhz) unscaled.
 * However Timers 2,3,4 get PCKL1 * 2 (36*2) = 72 Mhz.
 * So all the timers get a 72 Mhz clock the way I set things up.
 * And experiment bears this out for Timer 2.
 */

/* AN4776 is the 73 page STM32 Timer "cookbook" and seems like an
 * excellent document for understanding this timer.
 *
 */

/* Interrupt handlers */
void i2c1_ev_handler ( void ) { }
void i2c1_er_handler ( void ) { }
void i2c2_ev_handler ( void ) { }
void i2c2_er_handler ( void ) { }

static void
test_i2c ( void )
{
	nvic_enable ( I2C1_EV_IRQ );
	nvic_enable ( I2C1_ER_IRQ );
	nvic_enable ( I2C2_EV_IRQ );
	nvic_enable ( I2C2_ER_IRQ );
}

void
i2c_init ( void )
{
	test_i2c ();
}

/* THE END */
