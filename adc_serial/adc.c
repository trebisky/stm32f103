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
 */

/* One of the 2 adc units.
 * Some STM32 devices have 3, we only have 2
 */
struct adc {
	volatile unsigned long cr[2];
	volatile unsigned long idr;
	volatile unsigned long odr;
	volatile unsigned long bsrr;
	volatile unsigned long brr;
	volatile unsigned long lock;
};

#define ADC1_BASE	(struct adc *) 0x40012400
#define ADC2_BASE	(struct adc *) 0x40012800
#define ADC3_BASE	(struct adc *) 0x40013c00	/* We ain't got 3 of these tho */

/* ADC 1 and 2 share a common interrupt */
void
adc_handler ( void )
{
}

/* THE END */
