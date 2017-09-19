/* Usb.c
 * (c) Tom Trebisky  9-18-2017
 *
 * Driver for the STM32F103 USB
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

/* There are 32 of these 16 bytes objects in the USB_RAM
 *  This is 16*32 = 512 bytes, filling the USB_RAM area.
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

/* These are from table 63 in section 11 of the ref. manual */
#define	USB_HP_IRQ	19	/* high priority IRQ */
#define	USB_LP_IRQ	20	/* low priority IRQ */
#define	USB_WAKEUP_IRQ	42

void
usb_init ( void )
{
	struct usb *up = USB_BASE;

	printf ( "USB control: %08x\n", up->ctrl );
	printf ( "USB status: %08x\n", up->status );
}

/* THE END */
