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

/* Without special fiddling, the chip comes out of reset
 * with these peripheral clocks.
 */
#define PCLK1		8000000		/* APB1 clock */
#define PCLK2		8000000		/* APB2 clock */

#ifdef notdef
#define PCLK1		36000000		/* APB1 clock */
#define PCLK2		72000000		/* APB2 clock */
#endif

/* The baud rate register holds the divisor shifted 4 bits.
 * This lets the lower 4 bits be a fractional part.
 * The formula:  baud = clock / (16 * div) holds.
 * but if we let brr = 16 * div, we get:
 *   brr = clock / baud
 * Notice how absurdly simple this is.
 *  other code goes through silly gyrations with handling
 *  the whole and fractional part separately and even worse.
 */
static int
baud_calc ( int baud )
{
	return PCLK2 / baud;	/* XXX */
}

static void
uart_init ( struct uart *up, int baud )
{
	/* 1 start bit, even parity */
	up->cr1 = 0x340c;
	up->cr2 = 0;
	up->cr3 = 0;
	up->gtp = 0;
	up->baud = baud_calc ( baud );
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

	while ( ! (up->status & ST_TXE) )
	    ;
	up->data = c;
}

/* THE END */
