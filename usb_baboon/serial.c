/* serial.c
 * (c) Tom Trebisky  7-2-2017
 *
 * Driver for the STM32F103 usart
 */

// for cqueue
#include "kyulib.h"
#include "protos.h"

#define BIT(x)	(1<<x)

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

#define	UART1_IRQ	37
#define	UART2_IRQ	38
#define	UART3_IRQ	39

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

/* Bits in CR1 */
#define	C1_UE		BIT(13)		// Uart enable
#define	C1_M		BIT(12)		// 1 start, 9 data
#define	C1_PC		BIT(10)		// Parity
#define	C1_TE		BIT(3)		// enable Tx
#define	C1_RE		BIT(2)		// enable Rx

#define	C1_TXE		BIT(7)		// enable Tx empty interrupt
#define	C1_RXNE		BIT(5)		// enable Rx not empty interrupt

/* These must be maintained by hand */
#define PCLK1           36000000
#define PCLK2           72000000

static void
uart_init ( struct uart *up, int baud )
{
	/* 1 start bit, even parity */
	up->cr1 = 0x340c;
	up->cr2 = 0;
	up->cr3 = 0;
	up->gtp = 0;

	if ( up == UART1_BASE )
	    up->baud = PCLK2 / baud;
	else
	    up->baud = PCLK1 / baud;

	/* Enable interrupts */
	up->cr1 |= C1_TXE | C1_RXNE;

#ifdef notdef
	if ( up == UART1_BASE )
	    up->baud = get_pclk2() / baud;
	else
	    up->baud = get_pclk1() / baud;
#endif
}

static struct cqueue out_queue;

void
serial_init ( void )
{
	gpio_uart1 ();

	// uart_init ( UART1_BASE, 9600 );
	// uart_init ( UART1_BASE, 38400 );
	// uart_init ( UART1_BASE, 57600 ); ??
	uart_init ( UART1_BASE, 115200 );

	(void) cq_init ( &out_queue );

	nvic_enable ( UART1_IRQ );

#ifdef notdef
	gpio_uart2 ();
	uart_init ( UART2_BASE, 9600 );

	gpio_uart3 ();
	uart_init ( UART3_BASE, 9600 );
#endif
}

/* The original version prior to
 * interrupts and the cqueue.
 */
void
serial_putc_basic ( int c )
{
	struct uart *up = UART1_BASE;

	if ( c == '\n' )
	    serial_putc_basic ( '\r' );

	while ( ! (up->status & ST_TXE) )
	    ;
	up->data = c;
}

/* interrupt comes here.
 * interrupts are cleared by reading from
 * or writing to the data register.
 */
// void USART1_IRQ_Handler ( void )
void
uart1_handler ( void )
{
	struct uart *up = UART1_BASE;
	int c;

	if ( up->status & ST_RXNE ) {
	    c = up->data;
	    // Just read it and discard
	}

	if ( up->status & ST_TXE ) {
	    if ( cq_count ( &out_queue ) < 1 ) {
		up->cr1 &= ~C1_TXE;
		return;
	    }

	    c = cq_remove ( &out_queue );
	    up->data = c;
	}
}

/* enable TXE interrupt to start sending
 * what is in queue
 */
static inline void
serial_start ( void )
{
	struct uart *up = UART1_BASE;

	up->cr1 |= C1_TXE;
}

int
serial_pending ( void )
{
	return cq_count ( &out_queue );
}

void
serial_flush ( void )
{
	while ( cq_count ( &out_queue ) > 0 )
	    ;
}

void
serial_putc ( int c )
{
	disable_irq ();

	if ( c == '\n' )
	    cq_add ( &out_queue, '\r' );

	cq_add ( &out_queue, c );

	enable_irq ();

	// sort of brutal
	serial_start ();
}

/* Printf calls puts(), so it is a central choke point */

static int basic = 0;

static void
basic_puts ( char *s )
{
	while ( *s )
	    serial_putc_basic ( *s++ );
}

void
queue_puts ( char *s )
{
	while ( *s )
	    serial_putc ( *s++ );
}

void
serial_puts ( char *s )
{
	if ( basic )
	    basic_puts ( s );
	else
	    queue_puts ( s );
}

void serial_basic ( void ) { basic = 1; };
void serial_queue ( void ) { basic = 0; };

#ifdef notdef
/* see prf.c for these */
/* -------------------------------------------- */
/* Some IO stuff from Kyu */
/* -------------------------------------------- */

#define HEX(x)  ((x)<10 ? '0'+(x) : 'A'+(x)-10)

#define PUTCHAR(x)      *buf++ = (x)

static char *
shex2( char *buf, int val )
{
        PUTCHAR( HEX((val>>4)&0xf) );
        PUTCHAR( HEX(val&0xf) );
        return buf;
}

static char *
shex4( char *buf, int val )
{
        buf = shex2(buf,val>>8);
        return shex2(buf,val);
}

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
#endif

/* THE END */
