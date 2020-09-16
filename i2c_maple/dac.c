/* i2c_maple -- dac.c
 * (c) Tom Trebisky  9-15-2020
 *
 * This is what I call a "maple-ectomy"
 * The goal is to pull the i2c driver out of libmaple and
 *  make it work in my own environment.
 * I was able to successfully build the MCP4725 dac demo
 *  using the libmaple environment.  My idea was that the
 *  i2c code was C only and without too many dependencies.
 *
 * I began with my interrupt demo and then brought in the
 *  two files that contain the i2c driver.
 * I copied the include file collection lock stock and barrel
 *  in the libmaple subdirectory.
 *
 * This is turning into an interesting education.
 * The first big thing I encountered once I got it to compile
 *  and link was that my lds file had no allowance for the
 *  initialization of BSS and variables, so that needed to be
 *  addressed (It is not that hard, and needed to be done someday).
 */

void rcc_init ( void );
void led_init ( int );

void led_on ( void );
void led_off ( void );

/* By itself, with an 8 Mhz clock this gives a blink rate of about 2.7 Hz
 *  so the delay is about 185 ms
 * With a 72 Mhz clock this yields a 27.75 ms delay
 *  (confirmed using systick)
 */
void
delay ( void )
{
	volatile int count = 1000 * 200;

	while ( count-- )
	    ;
}

/* We scale the above to try to get a 500 ms delay */
/* Now a 75 ms delay with a 72 Mhz clock */
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

/* ------------------------------------------------ */
/* ------------------------------------------------ */
/* ------------------------------------------------ */

#include "libmaple/i2c.h"

#define MCP_ADDR         0x60
#define MCP_WRITE_DAC    0b01000000
#define MCP_WRITE_EEPROM 0b01100000
#define MCP_PD_NORMAL    0b00000000
#define MCP_PD_1K        0b00000010
#define MCP_PD_100K      0b00000100
#define MCP_PD_500K      0b00000110

static char write_msg_data[3];
static i2c_msg write_msg;

static char read_msg_data[5];
static i2c_msg read_msg;

static void
mcp_fail ( char *msg )
{
    serial_puts ( "MCP transaction fails\n" );
    serial_puts ( msg );
    serial_puts ( " fails\n" );
    serial_puts ( "Spinning\n" );

    for ( ;; ) ;
}

void mcp_i2c_setup(void) {
    write_msg.addr = MCP_ADDR;
    write_msg.flags = 0; // write, 7 bit address
    write_msg.length = sizeof(write_msg_data);
    write_msg.xferred = 0;
    write_msg.data = write_msg_data;

    read_msg.addr = MCP_ADDR;
    read_msg.flags = I2C_MSG_READ;
    read_msg.length = sizeof(read_msg_data);
    read_msg.xferred = 0;
    read_msg.data = read_msg_data;
}

void mcp_write_val(int val) {
    write_msg_data[0] = MCP_WRITE_DAC | MCP_PD_NORMAL;

    int tmp = val >> 4;
    write_msg_data[1] = tmp;
    tmp = (val << 4) & 0x00FF;
    write_msg_data[2] = tmp;

    if ( i2c_master_xfer(I2C2, &write_msg, 1, 0) )
	mcp_fail ( "MCP write" );
}

static int
mcp_read_val ( void )
{
    int tmp = 0;

    if ( i2c_master_xfer(I2C2, &read_msg, 1, 2) )
	mcp_fail ( "MCP read" );

    /* We don't care about the status and EEPROM bytes (0, 3, and 4). */
    tmp = (read_msg_data[1] << 4);
    tmp += (read_msg_data[2] >> 4);
    return tmp;
}

static int
mcp_test ( void )
{
    int val;
    int test_val = 0x0101;

    serial_puts ("Testing the MCP4725...\n");
    /* Read the value of the register (should be 0x0800 if factory fresh) */

    val = mcp_read_val();
    show32 ("DAC Register = 0x");

    mcp_write_val(test_val);
    show32 ("Wrote to the DAC: 0x");

    val = mcp_read_val();
    show32 ("DAC Register = 0x");

    if (val != test_val) {
        serial_puts ("ERROR: MCP4725 not responding correctly\n");
        return 0;
    }

    serial_puts ("MCP4725 seems to be working\n");
    return 1;
}

static int dout = 0;

static void
dac_demo ( void )
{
    serial_puts ( "Starting dac demo\n" );

    i2c_master_enable(I2C2, 0);
    delay();
    serial_puts ( " demo 1\n" );

    mcp_i2c_setup();
    serial_puts ( " demo 2\n" );

    if ( mcp_test() )
	return;

    serial_puts ("Starting sawtooth wave\n");

    for ( ;; ) {
	mcp_write_val(dout);

	dout += 50;
	if ( dout > 4095 )
	    dout = 0;
    }
}

void
spinner ( void )
{
	serial_puts ( "Spinning\n" );

	for ( ;; ) ;
}

void
spinner2 ( void )
{
	int c;

	serial_puts ( "Ready\n" );

	for ( ;; ) {
	    c = serial_getc ();
	    // i2c_debug ();
	}
}

/* handler for a simulated interrupt on IRQ 12 */
/* This must be hand edited in locore.s */
#define PORK_IRQ	12

void
pork ( void )
{
	serial_puts ( "------------- PORK!!\n" );
}

void
main ( void )
{
	unsigned int time;

	rcc_init ();
	nvic_initialize ();

	serial_init ();
	// rcc_show ();
	// scb_unaligned ();

	serial_putc ( '\n' );
	serial_puts ( "Starting\n" );

	// sys_set_pri ( -1, 0xf );
	systick_prio ();

	// an enum is indeed a 1 byte item.
	// show_n ( "Sizeof ENUM: ", sizeof(xyz) );
	// show_reg ( "CCR: ", 0xE000ED14 );

	// rcc_show ();

	// i2c_init ();

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

	// systick_init_int ( 0xffffff );

	/* Aiming for a 1000 Hz rate */
	// systick_init_int ( 72 * 1000 );

	// spinner ();
	// spinner2 ();
	// serial_test ();
	systick_init_milli ();

	// wait a while and see if we get some ticks
	delay ();
	systick_test ();

	nvic_enable ( PORK_IRQ );
	nvic_sim_irq ( PORK_IRQ );

	dac_demo ();

	/* Seems OK */
	for ( ;; ) {
	    time = systick_get ();
	    show_n ( "systicks: ", time );
	    big_delay ();
	}
}

/* THE END */
