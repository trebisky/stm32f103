/* serial.c
 * (c) Tom Trebisky  7-2-2017
 *
 * Driver for the STM32F103 usart
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

static void
uart_init ( struct uart *up, int baud )
{
	/* 1 start bit, even parity */
	up->cr1 = 0x340c;
	up->cr2 = 0;
	up->cr3 = 0;
	up->gtp = 0;

	if ( up == UART1_BASE )
	    up->baud = get_pclk2() / baud;
	else
	    up->baud = get_pclk1() / baud;
}

void
serial_init ( void )
{
	gpio_uart1 ();
	// uart_init ( UART1_BASE, 9600 );
	// uart_init ( UART1_BASE, 38400 );
	// uart_init ( UART1_BASE, 57600 ); ??
	uart_init ( UART1_BASE, 115200 );

#ifdef notdef
	gpio_uart2 ();
	uart_init ( UART2_BASE, 9600 );

	gpio_uart3 ();
	uart_init ( UART3_BASE, 9600 );
#endif
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

void
serial_puts ( char *s )
{
	while ( *s )
	    serial_putc ( *s++ );
}

/* -------------------------------------------- */
/* Some IO stuff from Kyu */
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
