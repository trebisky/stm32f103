/* blink.c
 * (c) Tom Trebisky  7-2-2017
 *
 */

void rcc_init ( void );
void led_init ( void );

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

/* To change GPIO, you must hack it in gpio.c */
#define PC13	13
#define PA5	5

#define NBLINKS		3

void
startup ( void )
{
	int i;

	rcc_init ();

	// led_init ( PC13 );
	// led_init ( PA5 );
	led_init ();

	for ( ;; ) {
	    for ( i=0; i<NBLINKS; i++ ) {
		// led_on ();
		led_off ();
		delay ();
		// led_off ();
		led_on ();
		delay ();
	    }

	    big_delay ();
	    big_delay ();
	}
}

/* THE END */
