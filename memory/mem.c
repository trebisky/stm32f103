/* mem.c
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

static void
ndelay ( int n )
{
	while ( n-- )
	    delay ();
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

static void
spin ( void )
{
	for ( ;; ) {
	    // led_show ();
	    ndelay ( 20 );
	}
}

/* This goes into BSS and needs to be zeroed */
#define BS	128
char in_bss[BS]; 

/* On the first cut, this was in flash at 0x080005dc
 * so the value is correct, but don't try to modify it
 * or you get a major fault.
 * After adding a .data section to the lds file
 * this goes to 0x2000008C, but with random contents.
 * I add the routine to copy from __text_end onward,
 * and it works once I make sure that .rodata follows
 * .data !!
 */
int xyz = 1234;

extern int __text_end;
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

	show_n ( "xyz = ", xyz );
	show_reg ( "xyz ... ", &xyz );

	init_vars ();
	show_reg ( "__text_end ", &__text_end );

	show_n ( "xyz = ", xyz );
	show_reg ( "xyz ... ", &xyz );

	spin ();
}

/* THE END */
