/* main.c
 *
 * This is usb_baboon, the third cut at working with
 * the papoon C++ framework.
 *
 * (c) Tom Trebisky  9-18-2017
 *
 * This uses a serial console, which I connect as follows:
 *  I use a CP2102 usb to serial gadget, which has 5 pins and
 *  works with 3.3 volt levels.  I make 3 connections:
 *  -- ground
 *  -- Tx on the CP2102 goes to A10 on the STM32
 *  -- Rx on the CP2102 goes to A9 on the STM32
 *
 * Notable things added to this code:
 *  -- add a basic printf taken from my Kyu project
 *
 * inter.c was kind of a test bed for a number of things:
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

#include "protos.h"

void rcc_init ( void );
void led_init ( void );

void led_on ( void );
void led_off ( void );

/* Sorta close to 1 ms with 72 Mhz processor.
 * It actually turns out to give 0.913 milliseconds
 * with the current compiler and the CPU at 72 Mhz.
 */
// #define DELAY_MAGIC 7273

/* some experiments along with examination of the
 * disassembled code reveal that the loop body
 * consists of 5 instructions that use 9 cycles
 * to run.  So this number is correct in theory,
 * but all bets are off if the compiler does better.
 */
#define DELAY_MAGIC 8000

static void
delay_one_ms ( void )
{
        volatile int count = 8000;

        while ( count-- )
            ;
}

#ifdef notdef
/* I currently (Nov, 2023) compile with -O2.  When I do this,
 * the compiler is clever enough to inline the above function,
 * which is certainly fine, though interesting.
 */
void
delay_ms ( int ms )
{
        while ( ms-- )
            delay_one_ms ();
}
#endif

void
delay_ms ( int ms )
{
        volatile int count = ms * 8000;

        while ( count-- )
            ;
}

void
delay_sec ( int sec )
{
        while ( sec-- )
            delay_ms ( 1000 );
}

#ifdef notdef
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
#endif

#define NBLINKS		2

/* Turn the LED on for a pulse */
static void
led_show ( void )
{
	led_on ();
	// delay ();
	delay_ms ( 500 );
	led_off ();
}

static void
led_demo ( void )
{
	int i;


	for ( ;; ) {
	    for ( i=0; i<NBLINKS; i++ ) {
		led_on ();
		// delay ();
		// delay ();
		delay_ms ( 50 );
		led_off ();
		// delay ();
		// delay ();
		delay_ms ( 50 );
	    }

	    delay_ms ( 1500 );
	    // big_delay ();
	    // big_delay ();
	    // big_delay ();
	}
}

extern volatile unsigned long systick_count;

void
startup ( void )
{
	int t;
	unsigned long systick_next;

	mem_init ();

	rcc_init ();

	serial_init ();
	// serial_basic ();

	printf ( " -- Booting ------------------------------\n" );
	printf ( "STM32 usb_baboon demo starting\n" );

	printf ( "AA\n" );	// nope

	// usb_init ();

	led_init ();
	printf ( "AA\n" );	// nope

	// Just USB stuff at this time
	gpio_init ();

	printf ( "AA\n" );	// nope

	led_on ();
	// led_off ();

	/* This gives us a 1 us interrupt rate !  */
	// systick_init_int ( 72 );
	/* This gives a 1 ms rate */
	systick_init_int ( 72 * 1000 );

	printf ( "STM32 usb_baboon demo\n" );

#ifdef notdef
	big_delay ();
	big_delay ();
	pma_show ();
#endif

	/* usb_init generally doesn't return */
	usb_init ();

	printf ( "Delay 10\n" );
	delay_sec ( 10 );

	serial_puts ( "Main is blinking\n" );
	led_demo ();

	serial_puts ( "Main is spinning\n" );
	for ( ;; )
	    ;
}

/* Could blink a tell-tale pattern */
void
panic ( char *msg )
{
	printf ( "Panic!! -- %s\n", msg );
	for ( ;; )
	    ;
}

/* THE END */
