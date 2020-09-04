/* lcd.c
 * (c) Tom Trebisky  9-2-2020
 *
 * i2c demo.
 * This began as the interrupt demo and added i2c code.
 *
 * I use the systick interrupt to blink the onboard LED as
 *  a sanity check.
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
	rcc_init ();
	serial_init ();

	serial_putc ( '\n' );
	serial_puts ( "Starting\n" );

	rcc_show ();

	i2c_init ();

	led_init ( PC13 );
	led_off ();

	/* This gives us a 1 us interrupt rate !
	 * So we toggle the output port at 500 kHz
	 */
	// systick_init_int ( 72 );

	/* Try for 1 Hz - 0x044aa200 won't fit in 24 bits */
	// systick_init_int ( 72*1000*1000 );

	/* This give a happy LED blink rate,
	 *  and confirms that some kind of interrupt
	 *  is getting handled properly
	 * The value is 16,777,215
	 */
	systick_init_int ( 0xffffff );
	show_n ( "Systick: ", 0xffffff );

	serial_puts ( "Spinning\n" );

	for ( ;; ) {
	}
}

/* THE END */
