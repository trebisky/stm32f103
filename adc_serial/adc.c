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
 */

/* One of the 2 adc units.
 * Some STM32 devices have 3, we only have 2
 */
struct adc {
	volatile unsigned long sr;
	volatile unsigned long cr1;
	volatile unsigned long cr2;
	volatile unsigned long smpr1;
	volatile unsigned long smpr2;
	volatile unsigned long jof1;
	volatile unsigned long jof2;
	volatile unsigned long jof3;
	volatile unsigned long jof4;
	volatile unsigned long htr;
	volatile unsigned long ltr;
	volatile unsigned long sqr1;
	volatile unsigned long sqr2;
	volatile unsigned long sqr3;
	volatile unsigned long jsqr;
	volatile unsigned long jdr1;
	volatile unsigned long jdr2;
	volatile unsigned long jdr3;
	volatile unsigned long jdr4;
	volatile unsigned long dr;
};

#define ADC1_BASE	(struct adc *) 0x40012400
#define ADC2_BASE	(struct adc *) 0x40012800
#define ADC3_BASE	(struct adc *) 0x40013c00	/* We ain't got 3 of these tho */

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

void adc_start ( void );
void adc_on ( void );

// #define ADC_SUPPLY 3130
#define ADC_SUPPLY 3600

/* ADC 1 and 2 share a common interrupt */
void
adc_handler ( void )
{
	struct adc *ap = ADC1_BASE;
	int x;
	int y;

	// printf ( "ADC interrupt\n" );
	// serial_puts ( "ADC interrupt\n" );

	if ( ap->sr & SR_EOC ) {
	    x = ap->dr;
	    y = ADC_SUPPLY * x / 4096;
	    printf ( "ADC eoc: %08x %d %d\n", x, x, y );
	} else {
	    serial_puts ( "unexpected ADC interrupt\n" );
	}
}

void
adc_init ( void )
{
	struct adc *ap = ADC1_BASE;
	int i;

	printf ( "ADC sr = %08x\n", ap->sr );
	printf ( "ADC cr2 = %08x\n", ap->cr2 );

	nvic_enable ( ADC_IRQ );

	/* Make A0 an analog input */
	gpio_a_analog ( 0 );

	adc_on ();
	ap->cr1 |= CR_EOCIE;
	delay_ms ( 20 );

	printf ( "ADC sr = %08x\n", ap->sr );
	printf ( "ADC cr2 = %08x\n", ap->cr2 );

	for ( i=0; i<10; i++ ) {
	    adc_start ();
	    delay_ms (1000);
	}
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
