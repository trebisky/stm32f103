/* adc.c
 * (c) Tom Trebisky  11-16-2017
 *
 * Driver for the STM32F103 ADC subsystem
 *
 * Described in chapter 11 of the STM32F103 reference manual.
 *
 * The ADC is on APB2.
 *
 * Note that pins must be configured for analog input,
 *  see gpio.c for this.
 *
 * The ADC can handle up to 18 muxed inputs.
 *  16 are external, 2 are internal.
 * The internal devices are:
 *	channel 16 - temp sensor
 *	channel 17 - voltage reference
 * These internal devices are only available to ADC1
 *
 * By all indications, only 10 of the 16 external inputs
 *  actually get routed to device pins.
 * Indeed, you do not get C0-C5 on the STM32F103,
 *  so only 10 channels are available.
 *
 * AIN0 = A0
 * AIN1 = A1
 * AIN2 = A2
 * AIN3 = A3
 * AIN4 = A4
 * AIN5 = A5
 * AIN6 = A6
 * AIN7 = A7
 * AIN8 = B0
 * AIN9 = B1
 * AIN10 = C0 -- unavailable
 * AIN11 = C1 -- unavailable
 * AIN12 = C2 -- unavailable
 * AIN13 = C3 -- unavailable
 * AIN14 = C4 -- unavailable
 * AIN15 = C5 -- unavailable
 * AIN16 = temp
 * AIN17 = vref
 *
 * The Datasheet says that inputs can be 0-3.6 volts
 * The absolute max is 4.0 volts.
 * My unit has a 3.3 volt regulator - but I measure 3.12 volts
 *
 * Conversions take 12.5 clocks.
 *  I run the ADC at PCKL2 / 6 = 12 Mhz
 * This is a game between the cpu clock, the prescaler options,
 *  and the requirement that the ADC clock be 14 Mhz or less.
 * We can have 1 μs at 56 MHz or 1.17 μs at 72 MHz
 *
 */

/* One of the 2 adc units.
 * Some STM32 devices have 3, we only have 2
 */
struct adc {
	volatile unsigned long sr;	/* status */
	volatile unsigned long cr1;	/* control 1 */
	volatile unsigned long cr2;	/* control 2 */
	volatile unsigned long smpr1;	/* sample times ch 10 .. 17 */
	volatile unsigned long smpr2;	/* sample times ch 0 .. 9 */
	volatile unsigned long jof1;	/* injected channel offsets */
	volatile unsigned long jof2;
	volatile unsigned long jof3;
	volatile unsigned long jof4;
	volatile unsigned long htr;	/* watchdog high */
	volatile unsigned long ltr;	/* watchdog low */
	volatile unsigned long sqr1;	/* sequence reg ch 13 .. 16 and Len-1 */
	volatile unsigned long sqr2;	/* sequence reg ch 7 .. 12 */
	volatile unsigned long sqr3;	/* sequence reg ch 1 .. 6 */
	volatile unsigned long jsqr;
	volatile unsigned long jdr1;	/* injected data register 1 */
	volatile unsigned long jdr2;	/* injected data register 2 */
	volatile unsigned long jdr3;	/* injected data register 3 */
	volatile unsigned long jdr4;	/* injected data register 4 */
	volatile unsigned long dr;	/* data register */
};

#define ADC1_BASE	(struct adc *) 0x40012400
#define ADC2_BASE	(struct adc *) 0x40012800
#define ADC3_BASE	(struct adc *) 0x40013c00	/* We ain't got no ADC3  */

#define	ADC_IRQ		18

/* Bits on the SR (only 5 of them) */
#define SR_AWD		0x01
#define SR_EOC		0x02
#define SR_JEOC		0x04
#define SR_JSTART	0x08
#define SR_START	0x10

/* Bits in CR1 */
#define CR_EOCIE	0x20		/* enable EOC interrupts */
#define CR_AWDIE	0x40		/* enable AWD interrupts */
#define CR_JEOCIE	0x80		/* enable JEOC interrupts */

/* Bits in CR2 */
#define CR_ON		0x1		/* turn on the ADC */
#define CR_CONT		0x2		/* continuous mode */
#define CR_CAL		0x4
#define CR_RSTCAL	0x8
#define CR_TSVREFE	0x800000	/* enable temp and vref */

#define CHAN_TEMP	16
#define CHAN_VREF	17

/* Vref is supposed to be 1.2 volts nominal
 *   1.16 to 1.26 volts.
 */

void adc_start ( void );
void adc_on ( void );
void adc_set_chan ( int );

/* It is not clear whether conversions are full scale at 3.6 volts
 * or at the actual Vcc voltage - it seems to be actual Vcc, maybe.
 */
// #define ADC_SUPPLY 3080
#define ADC_SUPPLY 3310
// #define ADC_SUPPLY 3600

static volatile char adc_finished; 
static short adc_val;

/* ADC 1 and 2 share a common interrupt */
void
adc_handler ( void )
{
	struct adc *ap = ADC1_BASE;
	// int x;
	// int y;

	// printf ( "ADC interrupt\n" );
	// serial_puts ( "ADC interrupt\n" );

	if ( ap->sr & SR_EOC ) {
	    // x = ap->dr;
	    // y = ADC_SUPPLY * x / 4096;
	    // printf ( "ADC eoc: %08x %d %d\n", x, x, y );
	    adc_val = ap->dr;
	    adc_finished = 1;
	} else {
	    serial_puts ( "unexpected ADC interrupt\n" );
	}
}

int
adc_read ( void )
{
	adc_finished = 0;

	adc_start ();

	while ( ! adc_finished )
	    ;

	return ADC_SUPPLY * adc_val / 4096;
}

void
adc_init ( void )
{
	struct adc *ap = ADC1_BASE;

	printf ( "ADC sr = %08x\n", ap->sr );
	printf ( "ADC cr2 = %08x\n", ap->cr2 );

	nvic_enable ( ADC_IRQ );

	/* Turn on the ADC with temp and vref enabled */
	ap->cr2 |= CR_ON|CR_TSVREFE;
	delay_ms ( 20 );

	/* Calibrate */
	ap->cr2 |= CR_CAL;
	while ( ap->cr2 & CR_CAL )
	    ;

	ap->cr1 |= CR_EOCIE;

	printf ( "ADC sr = %08x\n", ap->sr );
	printf ( "ADC cr2 = %08x\n", ap->cr2 );
}

/* Degenerate code for a single channel.
 *  Set the sequence registers.
 *  all the other channel registers get ignored.
 *  L = 0 says to convert one channel.
 */ 
void
adc_set_chan ( int chan )
{
	struct adc *ap = ADC1_BASE;

	ap->sqr1 = 0;
	ap->sqr2 = 0;
	ap->sqr3 = chan;
}

/* Start a conversion.
 * After that, it starts a conversion.
 */
void
adc_start ( void )
{
	struct adc *ap = ADC1_BASE;

	ap->cr2 |= CR_ON;
}

/* The first time this is called, it brings the ADC out of power-down.
 * After that, it would start a conversion.
 */
void
adc_on ( void )
{
	struct adc *ap = ADC1_BASE;

	ap->cr2 |= CR_ON;
}

/* power down the ADC */
void
adc_off ( void )
{
	struct adc *ap = ADC1_BASE;

	ap->cr2 &= ~CR_ON;
}

/* THE END */
