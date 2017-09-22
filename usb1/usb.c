/* Usb.c
 * (c) Tom Trebisky  9-18-2017
 *
 * Driver for the STM32F103 USB
 *
 * In general the USB module in the STM32F103 is a 16 bit device.
 * All the registers are 16 bit, and it talks to the USB_RAM using
 * a 16 bit bus.  This requires some care when making accesses from
 * the CPU.  The device registers can be accessed as 32 bit objects
 * that only have 16 active bits, so there is no confusion there.
 * However, the USB ram area can be confusing.  It presents a
 * contiguous series of 256 16-bit words to the USB, but when
 * viewed by the CPU, we see 256 16 bit words accessed as 32 bit
 * objects and so there are 16 bit "holes" interleaved with the
 * actual RAM.  
 */

struct usb {
	volatile unsigned long epr[8];	/* 00 - endpoint registers */
	    long	_pad0[8];
	volatile unsigned long ctrl;	/* 40 */
	volatile unsigned long status;	/* 44 - interrupt status */
	volatile unsigned long fnr;	/* 48 - frame number */
	volatile unsigned long daddr;	/* 4c - device address */
	volatile unsigned long btable;	/* 50 - buffer table */
};

/* The USB registers are 16 bit registers in 32 bit holes.
 */

/* Bits in the control register */
#define CTL_CTRM	0x8000	/* correct transfer interrupt mask */
#define CTL_OVRM	0x4000	/* PMA memory overrun int mask */
#define CTL_ERRM	0x2000	/* error int mask */
#define CTL_WKM		0x1000	/* wakeup int mask */
#define CTL_SUSPM	0x0800	/* suspend int mask */
#define CTL_RESETM	0x0400	/* reset int mask */
#define CTL_SOFM	0x0200	/* SOF int mask */
#define CTL_ESOFM	0x0100	/* ESOF int mask */

#define CTL_PWRDN	0x0002
#define CTL_RESET	0x0001

/* Bits in the status register */
#define INT_CTR		0x8000
#define INT_ERR		0x2000
#define INT_RESET	0x0400

/* Bits in an endpoint register */
#define EP_RX_VALID		0x3000
#define EP_TYPE_CONTROL		0x0200

#define CTL_ENABLE	CTL_ERRM | CTL_CTRM | CTL_RESETM
#define INT_ENABLE	(INT_ERR | INT_CTR | INT_RESET)

/* There can be 32 of these 8 byte objects in the USB_RAM
 * If we used them all, that would be 256 bytes,
 *  consuming half the USB_RAM.
 * Note that these look like 16 byte objects when viewed
 *  by the CPU, as discussed above.
 * Typically we don't set up all the endpoints, and
 *  the USB_RAM has to hold the endpoint buffers.
 * If the endpoint buffers are 64 bytes each,
 *   we have room for only 4 of them !!
 */
struct usb_desc {
	volatile unsigned long tx_addr;
	volatile unsigned long tx_count;
	volatile unsigned long rx_addr;
	volatile unsigned long rx_count;
};

/* The USB subsystem is described on pages 625-657 of
 *  the reference manual (section 23) */
#define USB_BASE	(struct usb *) 0x40005C00
#define USB_RAM		(char *) 0x40006000
#define USB_DESC	((struct usb_desc *) 0x40006000)

/* offsets into USB_RAM */
#define BTABLE_ADDR	0
#define ENDP0_RX_ADDR	0x40
#define ENDP0_TX_ADDR	0x80

#define USB_MAX_BUF	64	/* 0x40 */

/* These are from table 63 in section 11 of the ref. manual */
#define	USB_HP_IRQ	19	/* high priority IRQ */
#define	USB_LP_IRQ	20	/* low priority IRQ */
#define	USB_WAKEUP_IRQ	42

void
usb_init ( void )
{
	struct usb *up = USB_BASE;
	struct usb_desc *descp;

	/* set up clocks */
	/* de-assert reset */
	rcc_usb_reset ();

	/* This is an offset into USB_RAM */
	up->btable = BTABLE_ADDR;

	descp = &USB_DESC[0];
	descp->rx_addr = ENDP0_RX_ADDR;
	// descp->rx_count = USB_MAX_BUF;
	descp->rx_count = 0x1f << 10;
	descp->tx_addr = ENDP0_TX_ADDR;
	// descp->tx_count = USB_MAX_BUF;
	descp->tx_count = 0;

	printf ( "Endpoint reg 0 = %08x\n", up->epr[0] );

	up->epr[0] = EP_RX_VALID | EP_TYPE_CONTROL;

	printf ( "Endpoint reg 0 = %08x\n", up->epr[0] );

	nvic_enable ( USB_HP_IRQ );
	nvic_enable ( USB_LP_IRQ );
	nvic_enable ( USB_WAKEUP_IRQ );

	printf ( "USB control: %08x\n", up->ctrl );
	printf ( "USB status: %08x\n", up->status );

	/* clear powerdown and reset
	 * enable some interrupts.
	 */
	up->ctrl = CTL_ENABLE;

	printf ( "USB control: %08x\n", up->ctrl );
	printf ( "USB status: %08x\n", up->status );
}

/* USB uses 3 interrrupt vectors.
 * Here is the "high priority" interrupt
 */
void
usb_hp_handler ( void )
{
	struct usb *up = USB_BASE;
	printf ( "- USB high priority interrupt\n" );
	printf ( "USB status: %08x\n", up->status );
}

/* USB low priority interrupt.
 * With nothing plugged in, I see continual error interrupts,
 *  but they stop when I plug in a cable.
 */
void
usb_lp_handler ( void )
{
	struct usb *up = USB_BASE;
	int last;

	if ( up->status & INT_RESET ) {
	    up->status = ~INT_RESET;
	    last = up->status;
	    printf ( "USB reset interrupt, %08x\n", up->status );
	}

	if ( up->status & INT_ERR ) {
	    up->status = ~INT_ERR;
	    last = up->status;
	    printf ( "USB error interrupt, %08x\n", up->status );
	} 

	if ( last & INT_ENABLE ) {
	    printf ( "- USB low priority interrupt\n" );
	    printf ( "USB status: %08x\n", last );
	}

}

/* USB wakeup interrupt.
 */
void
usb_wk_handler ( void )
{
	struct usb *up = USB_BASE;
	printf ( "- USB wakeup interrupt\n" );
	printf ( "USB status: %08x\n", up->status );
}

/* THE END */
