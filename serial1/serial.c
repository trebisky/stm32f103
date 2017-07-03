/* serial.c
 * (c) Tom Trebisky  7-2-2017
 *
 * Driver for the STM32F103 usart
 */

/* One of the 3 usarts */
struct usart {
	volatile unsigned long status;
	volatile unsigned long data;
	volatile unsigned long baud;
	volatile unsigned long cr1;
	volatile unsigned long cr2;
	volatile unsigned long cr3;
	volatile unsigned long gtp;	/* guard time and prescaler */
};

#define USART1_BASE	(struct usart *) 0x40013800
#define USART2_BASE	(struct usart *) 0x40004400
#define USART3_BASE	(struct usart *) 0x40004800

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

void
serial_init ( void )
{
}

void
serial_putc ( int c )
{
	struct usart *up = USART1_BASE;

	while ( ! (up->status & ST_TXE) )
	    ;
	up->data = c;
}

/* THE END */
