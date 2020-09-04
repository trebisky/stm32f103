/* inter.c
 * (c) Tom Trebisky  7-7-2017
 *
 * Interrupt demo.
 * This sets up Timer 2 and then sends a sequence of characters
 * out on the Uart, one per timer tick.
 *
 * This has been kind of a test bed for a number of things:
 *
 *  -- getting the clock to run at 72 Mhz
 *  -- getting any kind of interrupt to work
 *  -- getting uart output
 *  -- getting the timer to do something
 *  -- getting output waveforms on some A port pin
 *  -- getting interrupts from sysclk
 *
 * Note that uart1 is on pins A9 and A10
 *  (it could alternately be on pins B6 and B7)
 *
 *  Uart 2 could be on pins A2 and A3
 *  Uart 3 could be on pins B10 and B11
 */

void rcc_init ( void );
void led_init ( int );

void led_on ( void );
void led_off ( void );

/* By itself, with an 8 Mhz clock this gives a blink rate of about 2.7 Hz
 *  so the delay is about 185 ms
 * With a 72 Mhz clock this yields a 27.75 ms delay
 */
void
delay ( void )
{
	volatile int count = 1000 * 200;

	while ( count-- )
	    ;
}

/* We scale the above to try to get a 500 ms delay */
void
big_delay ( void )
{
	volatile int count = 1000 * 540;

	while ( count-- )
	    ;
}

#define PC13	13
#define NBLINKS		2

/* Turn the LED on for a pulse */
static void
led_show ( void )
{
	led_on ();
	delay ();
	led_off ();
}

static void
led_demo ( void )
{
	int i;


	for ( ;; ) {
	    for ( i=0; i<NBLINKS; i++ ) {
		led_on ();
		delay ();
		led_off ();
		delay ();
	    }

	    big_delay ();
	}
}

void
startup ( void )
{
	int count = 0;
	int t;
	// int tmin = 0xffff;
	// int tmax = 0;

	rcc_init ();
	serial_init ();
	serial_putc ( '\r' );
	serial_putc ( '\n' );

#define A_BIT	7
	// gpio_a_init ( A_BIT );

	led_init ( PC13 );
	led_on ();

	/* This gives us a 1 us interrupt rate !
	 * So we toggle the output port at 500 kHz
	 */
	systick_init_int ( 72 );

/* With this timer running, I see a waveform on A1, A2, A3 that is
 * 2 us high, 2 us low (250 kHz).
 * The timer I set up is indeed running at 250 kHz and is tim2
 * These pins are:
 *  A1 = PWM2/2
 *  A2 = PWM2/3
 *  A3 = PWM2/4
 */
	timer_init ();

	// led_demo ();

#ifdef notdef
	/* This yields a 1.34 Mhz waveform with a 72 Mhz clock */
	for ( ;; ) {
	    led_on ();
	    led_off ();
	}
#endif

#ifdef notdef
	/* This runs a bit faster, yielding 1.44 Mhz */
	for ( ;; ) {
	    gpio_c_set ( PC13, 0 );
	    gpio_c_set ( PC13, 1 );
	}
#endif

#ifdef notdef
	led_on ();

	/* This gives a nice clean waveform at 1.24 Mhz */
	for ( ;; ) {
	    gpio_a_set ( A_BIT, 0 );
	    gpio_a_set ( A_BIT, 1 );
	}
#endif

	serial_puts ( "Spinning\n" );

	for ( ;; ) {
#ifdef notdef
	    if ( (++count % 16) == 0 )
		led_show ();
	    // serial_putc ( 'a' );
	    // serial_putc ( 'A' );
	    big_delay ();
	    /*
	    t = timer_get();
	    if ( t < tmin ) tmin = t;
	    if ( t > tmax ) tmax = t;
	    show16 ( "Min ", tmin );
	    show16 ( "Max ", tmax );
	    */
#endif
	}
}

/* THE END */
