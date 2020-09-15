/* Nvic.c
 * (c) Tom Trebisky  7-7-2017
 *
 * Driver for the STM32F103 NVIC interrupt controller
 *
 * The registers for this are part of the ARM Cortex M3
 *  "System Control Space" which contains:
 *
 *  0xE000E000 - interrupt type register
 *  0xE000E010 - system timer (SysTick)
 *  0xE000E100 - NVIC (nested vectored interrupt controller)
 *  0xE000ED00 - system control block (includes CPU ID)
 *  0xE000EF00 - software trigger exception registers
 *  0xE000EFD0 - ID space
 */

void
bogus_nmi ( void )
{
	serial_puts ( "Bogus NMI exception, spinning ... \n" );
	for ( ;; ) ;
}
void
bogus_hf ( void )
{
	serial_puts ( "Bogus HF exception, spinning ... \n" );
	for ( ;; ) ;
}
void
bogus_mm ( void )
{
	serial_puts ( "Bogus MM exception, spinning ... \n" );
	for ( ;; ) ;
}
void
bogus_bus ( void )
{
	serial_puts ( "Bogus BUS exception, spinning ... \n" );
	for ( ;; ) ;
}
void
bogus ( void )
{
	serial_puts ( "Bogus exception, spinning ... \n" );
	for ( ;; ) ;
}

void
bogus_irq ( void )
{
	serial_puts ( "Bogus IRQ, spinning ... \n" );
	for ( ;; ) ;
}

struct nvic {
	volatile unsigned long iser[3];	/* 00 */
	volatile unsigned long icer[3];	/* 0c */
	/* ... */
};

#define NVIC_BASE	((struct nvic *) 0xe000e100)

#define NUM_IRQ	68

void
nvic_enable ( int irq )
{
	struct nvic *np = NVIC_BASE;

	if ( irq >= NUM_IRQ )
	    return;

	np->iser[irq/32] = 1 << (irq%32);
}

/* -------------------------------------- */

/* ARM system control block */

struct scb {
	volatile unsigned int cpuid;	/* 00 */
	volatile unsigned int icsr;	/* 04 */
	volatile unsigned int vtor;	/* 08 */
	volatile unsigned int aircr;	/* 0c */
	volatile unsigned int scr;	/* 10 */
	volatile unsigned int ccr;	/* 14 */
	volatile unsigned int shp[12];	/* 18 */
	volatile unsigned int cshcsr;
	volatile unsigned int cfsr;
	volatile unsigned int hfsr;
	volatile unsigned int dfsr;
	volatile unsigned int mmfar;
	volatile unsigned int bfar;
};

#define SCB_BASE ((struct scb *) 0xE000ED00)

/* Allow unaligned accesses */
void
scb_unaligned ( void )
{
	struct scb *sp = SCB_BASE;

	sp->ccr |= 0x0008;	/* bit 3 */
}

/* -------------------------------------- */

/* The systick timer counts down to zero and then reloads.
 * It sets the flag each time it reaches 0.
 * This is a 24 bit timer (max reload is 0x00FFFFFF).
 *
 * The Calib register is used to get 10 ms timing
 *  in a way that is not clear to me.
 * As usual, documentation is bad and only experimentation
 *  will make things clear.
 * In the ref manual section 10 (interrupts and events)
 * section 10.1.1 says that the SysTick calibration register
 * is set to 9000 to obtain a 1ms system tick with a 9 Mhz clock.
 *
 * AN179 is helpful
 *  "Cortex M3 Embedded Software Development"
 *
 * With the INTPEND bit clear, the thing to do is to poll and
 *  watch the flag bit.  Reading the csr clears the flag bit.
 * Alternately the INTPEND bit can be set and the NVIC can
 *  be set up to let SysTick events cause interrupts.
 *
 * The interrupt IRQ assignment is described in the
 *  Ref Manual in section 10.1.2, be sure to use the last
 *  of the 3 tables (for "other" STM32F10xxx devices).
 * Note that SysTick is one of the 16 interrupt lines
 *  that are part of the ARM Cortex M3 interrupt set.
 */

struct systick {
	volatile unsigned long csr;	/* 00 */
	volatile unsigned long reload;	/* 04 */
	volatile unsigned long value;	/* 08 */
	volatile unsigned long calib;	/* 0C */
};

#define SYSTICK_BASE	((struct systick *) 0xe000e010)

#define	TICK_FLAG	0x10000
#define	TICK_SYSCLK	0x04	/* select sysclock */
#define	TICK_INTPEND	0x02
#define	TICK_ENABLE	0x01

/* This can be whatever you please */
#define SYSTICK_RELOAD	100

int ss = 0;

#define A_BIT	7

/* The actual handler is now in glue.c
 */
void
systick_handler_ORIG ( void )
{
	if ( ss ) {
	    // gpio_a_set ( A_BIT, 0 );
	    led_on ();
	    ss = 0;
	} else {
	    // gpio_a_set ( A_BIT, 1 );
	    led_off ();
	    ss = 1;
	}
}

void
systick_init ( int val )
{
	struct systick *sp = SYSTICK_BASE;

	sp->csr = TICK_SYSCLK;
	// sp->reload = SYSTICK_RELOAD - 1;
	sp->reload = val - 1;
	sp->value = 0;
	sp->csr = TICK_SYSCLK | TICK_ENABLE;
}

void
systick_init_int ( int val )
{
	struct systick *sp = SYSTICK_BASE;

	// show32 ( "systick: ", val );
	// show_n ( "Systick: ", val );

	systick_init ( val );
	sp->csr = TICK_SYSCLK | TICK_ENABLE | TICK_INTPEND;

	// gpio_a_init ( A_BIT );
}

void
systick_wait ( void )
{
	struct systick *sp = SYSTICK_BASE;

	while ( ! (sp->csr & TICK_FLAG) )
	    ;
}

/* THE END */
