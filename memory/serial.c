/* serial.c
 * (c) Tom Trebisky  7-2-2017
 *
 * Driver for the STM32F103 usart
 *
 * This began (2017) as a simple polled output driver for
 *  console messages on port 1
 * In 2020, I decided to extend it to listen to a GPS receiver
 *  on port 2.
 *
 * See section 27 (page 790) of the reference manual.
 *
 * Notice that these are numbered 1,2,3.
 * Uart 1 is on the APB2 bus (running at 72 Mhz).
 * Uart 2,3 are on the APB1 bus (running at 36 Mhz).
 */

/* One of the 3 uarts */
struct uart {
	volatile unsigned long status;	/* 00 */
	volatile unsigned long data;	/* 04 */
	volatile unsigned long baud;	/* 08 */
	volatile unsigned long cr1;	/* 0c */
	volatile unsigned long cr2;	/* 10 */
	volatile unsigned long cr3;	/* 14 */
	volatile unsigned long gtp;	/* 18 - guard time and prescaler */
};

#define UART1_BASE	(struct uart *) 0x40013800
#define UART2_BASE	(struct uart *) 0x40004400
#define UART3_BASE	(struct uart *) 0x40004800

/* Bits in Cr1 */
#define	CR1_ENABLE	0x2000
#define	CR1_9BIT	0x1000
#define	CR1_WAKE	0x0800
#define	CR1_PARITY	0x0400
#define	CR1_ODD		0x0200
#define	CR1_PIE		0x0100
#define	CR1_TXEIE	0x0080
#define	CR1_TCIE	0x0040
#define	CR1_RXIE	0x0020
#define	CR1_IDLE_IE	0x0010
#define	CR1_TXE		0x0008
#define	CR1_RXE		0x0004
#define	CR1_RWU		0x0002
#define	CR1_BRK		0x0001

// I don't understand the 9 bit thing, but it is needed.
#define CR1_CONSOLE	0x340c

/* SAM_M8Q gps is 9600, no parity, one stop */
#define CR1_GPS		0x200c

/* bits in the status register */
#define	ST_PE		0x0001
#define	ST_FE		0x0002
#define	ST_NE		0x0004
#define	ST_OVER		0x0008
#define	ST_IDLE		0x0010
#define	ST_RXNE		0x0020		/* Receiver not empty */
#define	ST_TC		0x0040		/* Transmission complete */
#define	ST_TXE		0x0080		/* Transmitter empty */
#define	ST_BREAK	0x0100
#define	ST_CTS		0x0200

void serial_putc ( int );

/* ------------------------------------------------------- */

#ifdef notdef
void
serial1_handler ( void )
{
}

void
serial2_handler ( void )
{
}

void
serial3_handler ( void )
{
	struct uart *up = UART3_BASE;
	int c;

	up->status = 0;

	c = up->data & 0x7f;
	serial_putc ( c );
}

#define SERIAL1_IRQ	37
#define SERIAL2_IRQ	38
#define SERIAL3_IRQ	39
#endif

/* ------------------------------------------------------- */

static void
uart_init ( struct uart *up, int baud )
{
	/* 1 start bit, even parity */
	if ( up == UART1_BASE )
	    up->cr1 = CR1_CONSOLE;
	else
	    up->cr1 = CR1_GPS;
	up->cr2 = 0;
	up->cr3 = 0;
	up->gtp = 0;

	if ( up == UART1_BASE )
	    up->baud = get_pclk2() / baud;
	else
	    up->baud = get_pclk1() / baud;
}

static void
uart_rx_enable ( struct uart *up )
{
	    up->cr1 |= CR1_RXIE;
}

void
serial_init ( void )
{
	gpio_uart1 ();
	// uart_init ( UART1_BASE, 9600 );
	// uart_init ( UART1_BASE, 38400 );
	// uart_init ( UART1_BASE, 57600 ); ??
	uart_init ( UART1_BASE, 115200 );

	/* SAM_M8Q gps is 9600, no parity, one stop */
	gpio_uart2 ();
	uart_init ( UART2_BASE, 9600 );

	gpio_uart3 ();
	uart_init ( UART3_BASE, 9600 );
}

void
serial_putc ( int c )
{
	struct uart *up = UART1_BASE;

	if ( c == '\n' )
	    serial_putc ( '\r' );

	while ( ! (up->status & ST_TXE) )
	    ;
	up->data = c;
}

/* Polled read (spins forever on console) */
int
serial_getc ( void )
{
	struct uart *up = UART1_BASE;

	while ( ! (up->status & ST_RXNE) )
	    ;
	return up->data & 0x7f;
}


void
serial_puts ( char *s )
{
	while ( *s )
	    serial_putc ( *s++ );
}

/* Quick and dirty */
int
serial2_getc ( void )
{
	struct uart *up = UART2_BASE;

	while ( ! (up->status & ST_RXNE) )
	    ;
	return up->data & 0x7f;
}

/* Quick and dirty */
int
serial3_getc ( void )
{
	struct uart *up = UART3_BASE;

	while ( ! (up->status & ST_RXNE) )
	    ;
	return up->data & 0x7f;
}

#ifdef notdef
/* For this test, I have a Sparkfun ublox sAM-M8Q GPS unit
 * connected with the Tx on the GPS going to A3 on the STM32
 * (serial 2, Rx).
 * To test with serial 3, Rx -  I can use pin B11
 *
 *  9-4-2020 I tested with both Serial 2 and 3.
 */
void
serial_test ( void )
{
	int c;

	nvic_enable ( SERIAL3_IRQ );
	uart_rx_enable ( UART3_BASE );
	for ( ;; ) ;

	/* Original polling test */
	for ( ;; ) {
	    // c = serial2_getc ();
	    c = serial3_getc ();
	    serial_putc ( c );
	}
}
#endif

/* -------------------------------------------- */
/* Some utility IO stuff from Kyu follows */
/* -------------------------------------------- */

#define HEX(x)  ((x)<10 ? '0'+(x) : 'A'+(x)-10)

#define PUTCHAR(x)      *buf++ = (x)

/* single byte as xx */
static char *
shex2( char *buf, int val )
{
        PUTCHAR( HEX((val>>4)&0xf) );
        PUTCHAR( HEX(val&0xf) );
        return buf;
}

/* "short" as xxyy */
static char *
shex4( char *buf, int val )
{
        buf = shex2(buf,val>>8);
        return shex2(buf,val);
}

/* "long" as aabbxxyy */
static char *
shex8( char *buf, int val )
{
        buf = shex2(buf,val>>24);
        buf = shex2(buf,val>>16);
        buf = shex2(buf,val>>8);
        return shex2(buf,val);
}

void
show16 ( char *s, int val )
{
	char buf[5];

	serial_puts ( s );
	shex4 ( buf, val );
	buf[4] = '\0';
	serial_puts ( buf );
	serial_putc ( '\n' );
}

static void
print32 ( int val )
{
	char buf[9];

	shex8 ( buf, val );
	buf[8] = '\0';
	serial_puts ( buf );
}

void
show32 ( char *s, int val )
{
	serial_puts ( s );
	print32 ( val );
	serial_putc ( '\n' );
}

void
show_reg ( char *msg, int *addr )
{
	serial_puts ( msg );
	serial_putc ( ' ' );
	print32 ( (int) addr );
	serial_putc ( ' ' );
	print32 ( *addr );
	serial_putc ( '\n' );
}

/* Just for fun, recursive base 10 print
 */
void
printn ( int x )
{
	int d;

	if ( x == 0 )
	    return;
	if ( x < 0 ) {
	    serial_putc ( '-' );
	    printn ( -x );
	    return;
	}
	d = x % 10;
	printn ( x / 10 );
	serial_putc ( '0' + d );
}

void
show_n ( char *s, int val )
{
	serial_puts ( s );
	serial_putc ( ' ' );
	printn ( val );
	serial_putc ( '\n' );
}

/* THE END */
