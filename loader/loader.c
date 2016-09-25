/* loader
 * Tom Trebisky  9-22-2016
 *
 * Program to talk to the serial boot loader on an STM32 chip.
 * In particular, I am developing and testing on a STM32F103C8T6
 *
 * **** This copy is wired up to do one and only one thing,
 * **** namely send the "disable readout protection" command
 * **** to the unit, this erases the flash and unlocks the chip.
 * **** It does NOT unlock the chip so this protocol can read it.
 * **** However, after this STLINK can read anyplace and
 * **** load code images to flash memory.
 *
 * The protocol is described in AN3155
 *
 * I was dissatified with the existing programs.
 * They either gave confusing errors (including traceback)
 *  and/or they lacked features to read memory areas.
 *
 * I tried initially to write this program in Ruby using
 *  rubyserial, but that just was not working out.
 * 
 * If this fails to start up and make a connection to the
 *  boot loader, be sure the BOOT0 jumper is in the "1" position.
 *
 * Here is a memory map of my chip from page 34 of the data sheet.
 *
 * 0x00000000 - 0x07ffffff - aliased to flash or sys memory depending on BOOT jumpers
 * 0x08000000 - 0x0800ffff - Flash (64K)
 * 0x1ffff000 - 0x1ffff7ff - Boot firmware in system memory
 * 0x1ffff800 - 0x1fffffff - option bytes
 * 0x20000000 - 0x20004fff - SRAM (20k)
 * 0x40000000 - 0x40023400 - peripherals
 *
 * The following area cannot be read or written by the boot rom
 * 0x20000000 - 0x20000200 - SRAM (512 bytes)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

char *port = "/dev/ttyUSB1";
int speed = B115200;

int serial_fd;

void error ( char * );

/* commands */
#define STM_INIT	0x7F

#define STM_GET		0x00	/* get version and commands */
#define STM_GET2	0x01	/* get version and protect status */
#define STM_CHIP	0x02	/* get chip ID */

#define STM_READ	0x11	/* read memory */
#define STM_UNK1	0x12	/* unknown (listed in my devices list of commands) */
#define STM_GO		0x21	/* Jump to flash or sram */
#define STM_WRITE	0x31	/* write flash or sram */

#define STM_ERASE	0x43	/* erase one to all pages */
#define STM_ERASE_EXT	0x44	/* erase one to all, extended */

#define STM_WPRO	0x63	/* write protect specified sectors */
#define STM_UNPROTECT	0x73	/* disable write protect for all sectors */

/* Mentioned in AN3155, not supported by my device */
#define STM_RPRO	0x82	/* enable readout protection */
#define STM_RPRO_DIS	0x92	/* disable readout protection */

/* Extended erase is only available for v3.x bootloaders and above.
 *  extended means a 2 byte address is allowed (for bigger devices?)
 */

/* responses */
#define STM_ACK		0x79
#define STM_NACK	0x1F

void
serial_setup ()
{
	struct termios termdata;

	serial_fd = open ( port, O_RDWR | O_NOCTTY | O_NDELAY );
	if ( serial_fd < 0 )
	    error ( "Cannot open serial port" );

	tcgetattr ( serial_fd, &termdata );

	// Baud rate
	cfsetispeed ( &termdata, speed );
	cfsetospeed ( &termdata, speed );

	// input modes - strip and check parity
	termdata.c_iflag &= ~( IXON | IXOFF | IXANY );
	termdata.c_iflag |= ( INPCK | ISTRIP );

	// output modes - no hokey pokey processing
	termdata.c_oflag &= ~OPOST;

	// control modes.

	// Stop bits, we want one, not two.
	termdata.c_cflag &= ~CSTOPB;

	// We want 8 bits.
	termdata.c_cflag &= ~CSIZE;
	termdata.c_cflag |= CS8;

	// parity enabled, even - not odd
        termdata.c_cflag |= PARENB;
        termdata.c_cflag &= ~PARODD;

	// ignore modem lines, enable receiver
	termdata.c_cflag |= ( CLOCAL | CREAD );

	// Disable HW and SW flow control (not in posix)
	termdata.c_cflag &= ~CRTSCTS;

	// local modes - this gives us raw input.
	termdata.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );

	// minimum characters for raw read
	// (so it will block until one character is ready).
	termdata.c_cc[VMIN] = 1;

	// timeout in deciseconds for raw read
	termdata.c_cc[VTIME] = 0;

	tcsetattr ( serial_fd, TCSANOW, &termdata );

	tcflush ( serial_fd, TCIOFLUSH );

	// Disable non-blocking stuff
	fcntl ( serial_fd, F_SETFL, 0 );
	// fcntl ( serial_fd, F_SETFL, O_NONBLOCK );
}

void
write_one ( int val )
{
	char vbuf = val;

	write ( serial_fd, &vbuf, 1 );
}

/* All commands are sent as the byte followed by its complement.
 *  a logical extension of the checksum on all blocks.
 */
void
write_cmd ( int cmd )
{
	char buf[2];

	buf[0] = cmd;
	buf[1] = ~cmd;

	write ( serial_fd, buf, 2 );
}

/* When you want to play fast and loose */
int
read_one ( void )
{
	char val;

	read ( serial_fd, &val, 1 );
	// return val & 0xff;
	return val;
}

/* Read with short timeout.
 * (10 milliseconds)
 */
int
read_f ( char *buf )
{
	struct timeval tv;
	fd_set ios;
	int rv;

	tv.tv_sec = 0;
	tv.tv_usec = 1000 * 10;

	FD_ZERO ( &ios );
	FD_SET ( serial_fd, &ios );
	rv = select ( serial_fd + 1, &ios, NULL, NULL, &tv );
	if ( rv < 1 )
	    return 0;

	return read ( serial_fd, buf, 1 );
}

/* Read with timeout */
int
read_t ( char *buf, int secs )
{
	struct timeval tv;
	fd_set ios;
	int rv;

	if ( secs > 0 ) {
	    tv.tv_sec = secs;
	    tv.tv_usec = 0;

	    FD_ZERO ( &ios );
	    FD_SET ( serial_fd, &ios );
	    rv = select ( serial_fd + 1, &ios, NULL, NULL, &tv );
	    if ( rv < 1 )
		return 0;
	}

	return read ( serial_fd, buf, 1 );
}

int
check_ack ( void )
{
	char ack;
	int n;

	n = read_t ( &ack, 0 );
	if ( n == 1 && ack == STM_ACK )
	    return 1;
	return 0;
}

int
stm_cmd ( int cmd )
{
	write_cmd ( cmd );
	return check_ack ();
}

unsigned char
checksum ( unsigned char *buf, int len )
{
	unsigned char rv = 0;
	int i;

	for ( i=0; i<len; i++ )
	    rv ^= buf[i];
	return rv;
}

/* Send block with checksum appended */
void
write_buf_sum ( char *buf, int len )
{
	write ( serial_fd, buf, len );
	write_one ( checksum ( (unsigned char *) buf, len ) );
}

void
write_addr ( unsigned int addr )
{
	char buf[4];
	
	buf[3] = addr & 0xff;
	addr <<= 8;
	buf[2] = addr & 0xff;
	addr <<= 8;
	buf[1] = addr & 0xff;
	addr <<= 8;
	buf[0] = addr & 0xff;
	
	write_buf_sum ( buf, 4 );
}

/*****************************************************************************/
/*****************************************************************************/

void
init_error ( char *msg )
{
	fprintf ( stderr, "%s\n", msg );
	// fprintf ( stderr, "Cannot communicate with bootloader\n" ); 
	fprintf ( stderr, "Be sure the BOOT0 jumper is set to 1\n" );
	fprintf ( stderr, "Press RESET on the target and try again\n" );
	exit ( 1 );
}

/* 1 second is more than adequate, it either responds immediately
 * or you are out of luck.
 */
#define INIT_TIMEOUT	1

void
error ( char *msg )
{
	fprintf ( stderr, "%s\n", msg );
	exit ( 1 );
}

void
stm_init ( void )
{
	char buf[80];
	char val;
	int n;

	/* Trying this twice works out well.
	 * It always gets it this way.
	 */
	write_one ( STM_INIT );
	n = read_f ( &val );
	if ( n == 0 ) {
	    write_one ( STM_INIT );
	    n = read_f ( &val );
	}
	if ( n == 0 )
	    init_error ( "Timeout initializing communication with bootloader" );

	/* Actually either ACK or NACK tell us we are talking
	 * to the boot loader !!
	 */
	if ( val == STM_ACK || val == STM_NACK )
	    return;

	// if ( val == STM_NACK )
	    // init_error ( "Received NACK from bootloader" );

	if ( val != STM_ACK ) {
	    sprintf ( buf, "Unexpected response (%02x) from bootloader", val );
	    init_error ( buf );
	}
}

/* Gets the boot loader version (2.2 in our case)
 * followed by an array of bytes, listing all of the
 * command bytes that it recognizes.
 */
int
stm_ver1 ( int verbose )
{
	int num;
	int i;
	int val;
	int rv;

	if ( stm_cmd ( STM_GET ) == 0 )
	    error ( "GET command rejected" );

	num = read_one ();
	for ( i=0; i <= num; i++ ) {
	    val = read_one ();
	    if ( i == 0 ) 
		rv = val;
	    if ( verbose )
		if ( i == 0 )
		    printf ( "Bootloader version = %02x\n", val );
		else
		    printf ( "Command recognized = %02x\n", val );
	}
	if ( check_ack() == 0 )
	    error ( "GET version ended badly" );
	return rv;
}

/* Gets the boot loader version (2.2 in our case)
 * followed by two zero bytes.
 * On some devices, this indicates if read protect
 * has been activated, and how many times.
 * On our device, this offers no extra information beyond
 * what you getfrom "ver1".
 */
#define NUM_VER2 3
int
stm_ver2 ( int verbose )
{
	int i;
	int val;
	int rv;

	if ( stm_cmd ( STM_GET2 ) == 0 )
	    error ( "GET2 command rejected" );

	for ( i=0; i < NUM_VER2; i++ ) {
	    val = read_one ();
	    if ( i == 0 ) 
		rv = val;
	    if ( verbose )
		if ( i == 0 )
		    printf ( "Bootloader version = %02x\n", val );
		else
		    printf ( "VER 2 = %02x\n", val );
	}
	if ( check_ack() == 0 )
	    error ( "GET version 2 ended badly" );
	return rv;
}

/* We get two bytes: 0x410 for our STM32F103 device */
int
stm_chip ( int verbose )
{
	int num;
	int i;
	int val;
	int rv = 0;

	if ( stm_cmd ( STM_CHIP ) == 0 )
	    error ( "CHIP command rejected" );

	num = read_one ();
	for ( i=0; i <= num; i++ ) {
	    val = read_one ();
	    if ( verbose )
		printf ( "CHIP = %02x\n", val );
		rv = rv << 8 | val;
	}
	if ( check_ack() == 0 )
	    error ( "GET chip ended badly" );
	return rv;
}

/* Just gets rejected on a factory device */
void
stm_unk ( void )
{
	if ( stm_cmd ( STM_UNK1 ) == 0 )
	    error ( "UNK command rejected" );
}

/* This is strange experimenting, but here is how it went.
 * With a fresh device, it would reject the READ command
 * without even looking at the address.  But then after
 * sending this command (which it seemed to take, even
 * though it is not in the command list), it will accept
 * the READ command (but has not yet read anything).
 * Also after sending this command, the unit no longer
 * flashes its LED (which was being done no doubt by an
 * application in flash memory, which was cleared).
 */
void
stm_unpro ( void )
{
	if ( stm_cmd ( STM_RPRO_DIS ) == 0 )
	    error ( "unpro command rejected" );
}

void
stm_read ( unsigned int addr )
{
	int count;
	int i;
	int val;

	if ( stm_cmd ( STM_READ ) == 0 )
	    error ( "READ command rejected" );

	write_addr ( addr );
	if ( check_ack() == 0 )
	    error ( "READ address rejected" );

	count = 128;
	val = count - 1;
	write_one ( val );
	write_one ( ~val );
	if ( check_ack() == 0 )
	    error ( "READ count rejected" );

#ifdef notdef
	count = 256;
	wbuf[0] = count-1;
	for ( i=0; i<count; i++ )
	    wbuf[i+1] = 0xbc;

	write_buf_sum ( wbuf, count+1 );
	if ( check_ack() == 0 )
	    error ( "WRITE buffer rejected" );
#endif
}


static char wbuf[257];

void
stm_write ( unsigned int addr )
{
	int count;
	int i;
	int val;

	if ( stm_cmd ( STM_WRITE ) == 0 )
	    error ( "WRITE command rejected" );

	write_addr ( addr );
	if ( check_ack() == 0 )
	    error ( "WRITE address rejected" );

	count = 256;
	wbuf[0] = count-1;
	for ( i=0; i<count; i++ )
	    wbuf[i+1] = 0xbc;

	write_buf_sum ( wbuf, count+1 );
	if ( check_ack() == 0 )
	    error ( "WRITE buffer rejected" );
}

void
stm_go ( unsigned int addr )
{
	if ( stm_cmd ( STM_GO ) == 0 )
	    error ( "GO command rejected" );

	write_addr ( addr );
	if ( check_ack() == 0 )
	    error ( "GO address rejected" );
}

#define SYS_BASE	0x1ffff000
#define FLASH_BASE	0x08000000
#define SRAM_BASE	0x20000000
#define SRAM_BASE2	0x20000200

int verbose = 0;

int
main ( int argc, char **argv )
{
	int chip;
	int version;

	serial_setup ();
	stm_init ();
	// printf ( " Initialized!\n" );

	version = stm_ver1 ( verbose );
	// (void) stm_ver2 ( verbose );
	chip = stm_chip ( verbose );

	printf ( "Boot loader version: %d.%d\n", version/16, version & 0xf );
	printf ( "Chip = %04x\n", chip );

	// NONE of these have yet to work.
	// stm_read ( SYS_BASE );
	// stm_read ( SRAM_BASE );
	// stm_read ( SRAM_BASE2 );
	// stm_read ( FLASH_BASE );

	// stm_write ( SRAM_BASE2 );
	// stm_write ( FLASH_BASE );

	stm_unpro ();

	// this works (at least no protocol errors)
	// stm_go ( FLASH_BASE );

	return 0;
}

/* THE END */
