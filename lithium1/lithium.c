/* lithium.c
 *
 * (c) Tom Trebisky  11-18-2017
 *
 * Derived from my usb1.c
 * which was derived from inter.c
 *
 * My goal here is to write a driver for the STM32 adc
 * I don't have working USB code yet, so I will need
 * to communicate via serial.
 *
 * This uses a serial console, which I connect as follows:
 *  I use a CP2102 usb to serial gadget, which has 5 pins and
 *  works with 3.3 volt levels.  I make 3 connections:
 *  -- ground
 *  -- Tx on the CP2102 goes to A10 on the STM32
 *  -- Rx on the CP2102 goes to A9 on the STM32
 *
 * Note that the board does not need to be powered via USB.
 * It is sufficient to power it from the ST-LINK.
 * So I have two USB cables during development.
 *  -- one to the ST-Link
 *  -- one to the CP2102 serial/usb gadget
 *
 * Notable things added to this code:
 *  -- add character read features to serial.c
 *
 * Note that uart1 is on pins A9 and A10
 *  (it could alternately be on pins B6 and B7)
 *
 *  Uart 2 could be on pins A2 and A3
 *  Uart 3 could be on pins B10 and B11
 */

void rcc_init ( void );
void adc_init ( void );
void led_init ( int );

void led_on ( void );
void led_off ( void );

/* These are just stubs because I am too lazy to comment out
 * the calls in locore.s
 */
void usb_hp_handler ( void ) {}
void usb_lp_handler ( void ) {}
void usb_wk_handler ( void ) {}

/* We have an LED on this pin */
#define PC13	13

/* We use A2 to enable the test */
#define ENABLE	2

/* Sorta close to 1 ms with 72 Mhz processor */
static void
delay_one_ms ( void )
{
	volatile int count = 7273;

	while ( count-- )
	    ;
}

void
delay_ms ( int ms )
{
	while ( ms-- )
	    delay_one_ms ();
}

extern volatile unsigned long systick_count;

#define CHAN_TEMP	16
#define CHAN_VREF	17

/* I have a resistor divider sampling the battery voltage.
 * I use a 4.7K and a 10K resistor, ordinary precision.
 *    This should scale by 0.680
 * With a 3.78 volt battery, I measure 2.576 volts
 *    This is a scale of 0.6815
 */
#define SCALE	681

void
relay_closed ( void )
{
	gpio_a_set ( ENABLE, 1 );
	led_on ();
	delay_ms ( 500 );
}

void
relay_open ( void )
{
	gpio_a_set ( ENABLE, 0 );
	led_off ();
	delay_ms ( 500 );
}

int
read_avg ( int n )
{
	int i;
	int val;
	int sum;

	sum = 0;
	for ( i=0; i<n; i++ ) {
	    val = adc_read ();
	    val = (val * 1000) / SCALE;
	    sum += val;
	    delay_ms (50);
	}

	return sum / n;
}

/* We have a load resistor of 16.67 ohms.
 * With the load attached the current is Vbat / 16.67
 * which for a 3.7 volt battery is 0.222 amps
 * This routine measure the voltage drop when we
 * attach the load.  Rint = Vdrop / Iload or
 * Rint = Vdrop * 16.67 / Vbat
 * Scaling Rload by 1000 yields milliohms
 */
#define NAVG	20
#define RLOAD	16667

void
measure_rint ( void )
{
	int v_open;
	int v_load;
	int dv;
	int rint;

	/* Make A0 an analog input */
	gpio_a_analog ( 0 );
	adc_set_chan ( 0 );

	relay_open ();

	v_open = read_avg ( NAVG );
	printf ( "V open = %d\n", v_open );

	relay_closed ();

	v_load = read_avg ( NAVG );
	printf ( "V loaded = %d\n", v_load );

	relay_open ();

	dv = v_open - v_load;
	rint = dv * RLOAD / v_load;
	// rint /= 100;
	printf ( "R int = %d (milliohms)\n", rint );
}

void
adc_test ( void )
{
	int i;
	int val;

	/* Make A0 an analog input */
	gpio_a_analog ( 0 );
	adc_set_chan ( 0 );

	printf ( " ADC channel 0\n" );

	for ( i=0; i<5; i++ ) {
	    val = adc_read ();
	    val = (val * 1000) / SCALE;
	    printf ( "Read: %d\n", val );
	    delay_ms (1000);
	}

	relay_closed ();

	for ( i=0; i<5; i++ ) {
	    val = adc_read ();
	    val = (val * 1000) / SCALE;
	    printf ( "Read: %d\n", val );
	    delay_ms (1000);
	}

	relay_open ();

	for ( i=0; i<5; i++ ) {
	    val = adc_read ();
	    val = (val * 1000) / SCALE;
	    printf ( "Read: %d\n", val );
	    delay_ms (1000);
	}

#ifdef notdef
	printf ( " temp\n" );
	adc_set_chan ( CHAN_TEMP );
	for ( i=0; i<10; i++ ) {
	    adc_start ();
	    delay_ms (1000);
	}

	printf ( " vref\n" );
	adc_set_chan ( CHAN_VREF );
	for ( i=0; i<10; i++ ) {
	    adc_start ();
	    delay_ms (1000);
	}
#endif
}

static int
my_strlen ( char *s )
{
	int rv = 0;

	while ( *s++ )
	    rv++;
	return rv;
}

void
startup ( void )
{
	int t;
	unsigned long systick_next;
	int x;
	char buf[80];
	char *p;

	rcc_init ();

	serial_init ();
	printf ( " -- Booting ------------------------------\n" );

	adc_init ();

	led_init ( PC13 );

	gpio_a_output ( ENABLE );

	/* This gives us a 1 us interrupt rate !  */
	// systick_init_int ( 72 );

	/* This gives a 1 ms rate */
	// systick_init_int ( 72 * 1000 );

	systick_init_int ( 72 * 1000 * 2000 );

	/* The board powers up with the port in
	 * the low state, so this actually does nothing.
	 * But we want to ensure the port is low, so
	 * the bipolar transistor that controls the relay
	 * comes up open -- we will close it when we are
	 * good and ready.
	 *
	 * We use the on board LED to indicate relay state.
	 */
	relay_open ();

#ifdef notdef
	x = 0;
	for ( ;; ) {
	    delay_ms ( 1000 );
	    if ( x ) {
		gpio_a_set ( ENABLE, 1 );
		x = 0;
	    } else {
		gpio_a_set ( ENABLE, 0 );
		x = 1;
	    }
	}
#endif

	// adc_test ();
	measure_rint ();

#ifdef notdef
	/* Wait for a command */
	serial_getl ( buf );

	// x = 1;
	x = my_strlen ( buf );
	for ( ;; ) {
	    // printf ( "cmd: %d %s\n", x, buf );
	    printf ( "---\n" );
	    for ( p=buf; *p; p++ ) 
		printf ( " %02x\n", *p );
	    delay_ms ( 2000 );
	    // x++;
	}
#endif

	serial_puts ( "Main is spinning\n" );

	for ( ;; )
	    ;
}

/* THE END */
