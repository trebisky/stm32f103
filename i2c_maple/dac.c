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

/* ------------------------------------------------ */
/* ------------------------------------------------ */
/* ------------------------------------------------ */

void i2c1_ev_handler () { serial_puts ( "1_ev\n" ); }
void i2c1_er_handler () { serial_puts ( "1_er\n" ); }
void i2c2_ev_handler () { serial_puts ( "2_ev\n" ); }
void i2c2_er_handler () { serial_puts ( "2_er\n" ); }

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

    i2c_master_xfer(I2C2, &write_msg, 1, 0);
}

static int
mcp_read_val ( void )
{
    int tmp = 0;

    i2c_master_xfer(I2C2, &read_msg, 1, 2);

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

enum bogus { FISH, CAT } xyz;

void
startup ( void )
{
	rcc_init ();
	serial_init ();
	rcc_show ();
	scb_unaligned ();

	serial_putc ( '\n' );
	serial_puts ( "Starting\n" );

	show_n ( "Sizeof ENUM: ", sizeof(xyz) );
	show_reg ( "CCR: ", 0xE000ED14 );

	rcc_show ();

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
	systick_init_int ( 72 * 1000 );

	// spinner ();
	// spinner2 ();
	// serial_test ();
	dac_demo ();
}

/* THE END */
