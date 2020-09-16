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

void sys_set_pri ( int, int );

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

#ifdef notdef
struct nvic {
	volatile unsigned long iser[3];	/* 00 */
	volatile unsigned long icer[3];	/* 0c */
	/* ... */
};
#endif

/* NVIC registers */
struct nvic {
	volatile unsigned int iser[8];	/* 00 */
	int _pad1[24];
	volatile unsigned int icer[8];	/* - */
	int _pad2[24];
	volatile unsigned int ispr[8];	/* - */
	int _pad3[24];
	volatile unsigned int icpr[8];	/* - */
	int _pad4[24];
	volatile unsigned int iabr[8];	/* - */
	int _pad5[56];
	volatile unsigned char prio[240];	/* - */
	int _pad6[644];
	volatile unsigned int stir;	/* software trigger */
};

#define NVIC_BASE	((struct nvic *) 0xe000e100)

#define NUM_IRQ	68

/* maple has an nvic_init inline thing in nvic.h
 *  so I we will call this "initialize"
 */
void
nvic_initialize ( void )
{
	struct nvic *np = NVIC_BASE;
	int i;

	// nvic_set_vector_table(address, offset);

	/* disable all interrupts */
	np->icer[0] = 0xffffffff;
	np->icer[1] = 0xffffffff;
	np->icer[2] = 0xffffffff;

	/* Put everything at the lowest priority level */
	for ( i=0; i<NUM_IRQ; i++ )
	    np->prio[i] = 0xf0;

	// nvic_irq_set_priority(NVIC_SYSTICK, 0xF);
	// sys_set_pri ( -1, 0xf );
}

/* This should yield a simulated interrupt on whatever IRQ
 *  is requested.  Tested - it works fine, but don't forget
 *  to first call nvic_enable() on the desired IRQ.
 */
void
nvic_sim_irq ( int irq )
{
	struct nvic *np = NVIC_BASE;

	np->stir = irq;
}

void
nvic_set_pri ( int irq, int prio )
{
	struct nvic *np = NVIC_BASE;

	np->prio[irq] = (prio & 0xf) << 4;
}

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
	volatile unsigned char shp[12];	/* 18 */
	volatile unsigned int cshcsr;
	volatile unsigned int cfsr;
	volatile unsigned int hfsr;
	volatile unsigned int dfsr;
	volatile unsigned int mmfar;
	volatile unsigned int bfar;
};

#define SCB_BASE ((struct scb *) 0xE000ED00)

/* The SHP registers hold the priority for system exceptions
 * at least for exception 4 through 15.
 * So, the following values index the SHP registers,
 * missing values are reserved.
 * Certain software (including maple) uses negative IRQ numbers
 * for these things, so SYSTICK is -1, NMI is -14.
 * I ignore that, but notice that the following will convert
 * one of these negative IRQ numbers into an exception index.
 *
 *	int index = ((unsigned int)sys_irq & 0xf);
 *      subtract 4 from this to get an index into shp[]
 *
 * The ARM documents also show these as 3 32 bit registers,
 * but it seems that byte access is allowed.
 */

#define EXC_RESET	1
#define EXC_NMI		2
#define EXC_HF		3
#define EXC_MM		4
#define EXC_BUS		5
#define EXC_USAGE	6
#define EXC_SYSCALL	11
#define EXC_PENDSV	14
#define EXC_SYSTICK	15
#define EXC_IRQ		16

/* Small numbers seem to be the highest priority.
 * Notice that the ARM documents seem to indicate than any value
 * from 0-255 may be set, unlike the IRQ priority values.
 */


void
sys_set_pri ( int exc, int prio )
{
	struct scb *sp = SCB_BASE;

	sp->shp[exc-4] = (prio & 0xf) << 4;
}

void
systick_prio ( void )
{
	struct scb *sp = SCB_BASE;
	volatile unsigned int *p;

	sys_set_pri ( EXC_SYSTICK, 0xf );

	p = &sp->ccr;
	p++;
	show_reg ( "SHP0 ", p );
	p++;
	show_reg ( "SHP1 ", p );
	p++;
	show_reg ( "SHP2 ", p );
}

/* As near as I can tell this is NOT what you want.
 * normally (with this not set) unaligned accesses
 * are allowed.  If you set this bit, they will cause
 * a fault.
 */
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

#ifdef notdef
int ss = 0;

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
#endif

/* This name is used by macros in libmaple */
volatile unsigned int systick_uptime_millis;

void
systick_handler ( void )
{
        systick_uptime_millis++;
}

unsigned int
systick_get ( void )
{
        return systick_uptime_millis;
}

void
systick_init ( int val )
{
	struct systick *sp = SYSTICK_BASE;

	sp->csr = TICK_SYSCLK;
	sp->reload = val - 1;
	sp->value = 0;
	sp->csr = TICK_SYSCLK | TICK_ENABLE;
}

void
systick_test ( void )
{
	if ( ! systick_uptime_millis ) {
	    serial_puts ( "Systick is not running\n" );
	    serial_puts ( "Spinning\n" );
	    for ( ;; ) ;
	}
	show_n ( "Systick count: ", systick_uptime_millis );
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
systick_init_milli ( void )
{
	systick_init_int ( 72 * 1000 );
}

void
systick_wait ( void )
{
	struct systick *sp = SYSTICK_BASE;

	while ( ! (sp->csr & TICK_FLAG) )
	    ;
}

/* THE END */
