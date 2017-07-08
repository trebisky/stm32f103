/* inter.c
 * (c) Tom Trebisky  7-7-2017
 *
 * Interrupt demo.
 * This sets up Timer 2 and then sends a sequence of characters
 * out on the Uart, one per timer tick.
 *
 */

void rcc_init ( void );
void led_init ( int );

void led_on ( void );
void led_off ( void );

/* By itself, this gives a blink rate of about 2.7 Hz
 *  so the delay is about 185 ms
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
show16 ( char *s, int val )
{
}

void
startup ( void )
{
	int count = 0;

	rcc_init ();
	serial_init ();
	serial_putc ( '\r' );
	serial_putc ( '\n' );

	led_init ( PC13 );
	timer_init ();

	// led_demo ();

	for ( ;; ) {
	    if ( (++count % 16) == 0 )
		led_show ();
	    // serial_putc ( 'a' );
	    // serial_putc ( 'A' );
	    big_delay ();
	    show16 ( "Timer ", timer_get() );
	}
}

/* THE END */
