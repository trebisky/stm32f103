/* afio.c
 * (c) Tom Trebisky  9-10-2020
 *
 * Alternate function gadget driver
 *  a close brother to gpio, sort of.
 *  See section 9.3 in the RM
 */

struct afio {
	volatile unsigned long cr;	/* 00 */
	volatile unsigned long mapr;	/* 04 */
	volatile unsigned long exticr1;	/* 08 */
	volatile unsigned long exticr2;	/* 0c */
	volatile unsigned long exticr3;	/* 10 */
	volatile unsigned long exticr4;	/* 14 */
	int	_pad;
	volatile unsigned long mapr2;	/* 1c */
};

#define AFIO_I2C1_REMAP	0x00000002
#define AFIO_SPI1_REMAP	0x00000001

#define AFIO_BASE	(struct afio *) 0x40010000

/* The purpose of this thing is not to enable or disable
 * alternate functions, but to determine which pins they
 * get assigned to if there is a choice.
 *
 * So in the case of i2c, i2c2 has no choice, the pins are
 * fixed on PB10 and PB11, so the AFIO does not get involved.
 * In the case of i2c1 we get to choose.
 * The default is pins PB6 and PB7,
 * but if you want PB8 and PB9, you need to set a bit
 * in an AFIO register.
 */

void
afio_init ( void )
{
    /* The AFIO has its own clock and reset ! */
    /* See rcc.c */
}

void
afio_i2c1_remap ( int alternate )
{
    struct afio *ap = AFIO_BASE;

    if ( alternate )
	ap->mapr |= AFIO_I2C1_REMAP;
    else
	ap->mapr &= ~AFIO_I2C1_REMAP;
}

/* THE END */
