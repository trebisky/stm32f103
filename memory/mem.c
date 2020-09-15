/* talk.c
 * (c) Tom Trebisky  7-2-2017
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

#define BS	128

char in_bss[BS]; 

void
main ( void )
{
	// int count = 0;
	int *p;

	rcc_init ();
	serial_init ();

	led_init ( PC13 );

	// led_demo ();

#ifdef notdef
	for ( ;; ) {
	    if ( (++count % 16) == 0 )
		led_show ();
	    serial_putc ( 'a' );
	    serial_putc ( 'A' );
	    delay ();
	}
#endif
	serial_puts ( "Starting ...\n" );
	for ( p = (int *) in_bss; p < (int *) &in_bss[BS]; p++ )
	    show_reg ( "In BSS: ", p );
}

/* THE END */
