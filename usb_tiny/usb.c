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
 *
 * However, the USB ram area can be confusing.  It presents a
 * contiguous series of 256 16-bit words to the USB, but when
 * viewed by the CPU, we see 256 16 bit words accessed as 32 bit
 * objects and so there are 16 bit "holes" interleaved with the
 * actual RAM.  
 */

#define NUM_EP		8

// static void * memset ( char *p, int val, int n ) { }

struct usb {
	volatile unsigned long epr[NUM_EP];	/* 00 - endpoint registers */
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
#define INT_OVR		0x4000
#define INT_ERR		0x2000
#define INT_WKUP	0x1000
#define INT_SUSP	0x0800
#define INT_RESET	0x0400
#define INT_SOF		0x0200
#define INT_ESOF	0x0100
#define INT_DIR		0x0010
#define INT_EP_MASK	0x000f

#define DADDR_0		0
#define DADDR_EF	0x80	/* enable function */

/* Bits in an endpoint register */
#define EP_RX_CTR		0x8000
#define EP_RX_DTOG		0x4000
#define EP_RX_MASK		0x3000
#define EP_RX_DISABLE		0x0000
#define EP_RX_STALL		0x1000
#define EP_RX_NAK		0x2000
#define EP_RX_VALID		0x3000
#define EP_SETUP		0x0800
#define EP_TYPE_MASK		0x0600
#define EP_TYPE_CONTROL		0x0200
#define EP_KIND			0x0100
#define EP_TX_CTR		0x0080
#define EP_TX_DTOG		0x0040
#define EP_TX_MASK		0x0030
#define EP_TX_DISABLE		0x0000
#define EP_TX_STALL		0x0010
#define EP_TX_NAK		0x0020
#define EP_TX_VALID		0x0030
#define EP_ADDR			0x000f

/* These are all the "toggle" bits in an endpoint register */
#define EP_TOGGLES		(EP_RX_DTOG | EP_RX_MASK | EP_TX_DTOG | EP_TX_MASK)

/* CTR = Correct transfer */

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
#define USB_RAM		(unsigned long *) 0x40006000
#define USB_DESC	((struct usb_desc *) 0x40006000)
#define USB_RAM_SIZE	512	/* bytes */

/* offsets into USB_RAM */
#define BTABLE_ADDR	0
#define ENDP0_RX_ADDR	0x40
#define ENDP0_TX_ADDR	0x80

#define USB_MAX_BUF	64	/* 0x40 */

#define RX_BLOCK_2	0x0000
#define RX_BLOCK_32	0x8000

/* We allocate one 32 byte block for Rx data */
#define RX_BLOCK_SIZE	1
#define RX_BLOCK_SHIFT	10

/* These are from table 63 in section 11 of the ref. manual */
#define	USB_HP_IRQ	19	/* high priority IRQ */
#define	USB_LP_IRQ	20	/* low priority IRQ */
#define	USB_WAKEUP_IRQ	42

void	clear_usb_ram ( void );
void	dump_usb_ram ( void );
static void	pma_show ( void );

/* Simulate a USB disconnect to get the hosts attention.
 * On our board, PA11 is USB D- and PA12 is USB D+
 * We yank D+ low for a while to do a disconnect.
 * XXX - it is not clear that this does anything.
 */
#define	DISCONNECT_PIN	12

void
usb_disconnect ( void )
{
	volatile unsigned int delay = 512;

	gpio_a_output ( DISCONNECT_PIN );
	gpio_a_set ( DISCONNECT_PIN, 0 );

	while ( delay-- )
	    ;

	gpio_a_input ( DISCONNECT_PIN );
}

/* The endpoint registers have "toggle" bits for some fields.
 * I have no idea what sort of crack the hardware designers
 *  were smoking when they set things up this way, but we
 *  are stuck with it.  So when you want to change the value
 *  in one of these toggle fields, you are faced with the
 *  following, namely 3 sorts of bits.
 *
 * 1 - there are non-toggle bits that you want to leave alone.
 * 2 - there are toggle bits that you want to leave alone.
 * 3 - there are toggle bits that you want to change.
 * 
 * So, we have to stand on our heads to write a value to these fields.
 */

static void
ep_toggle_write ( int ep, int val, int mask )
{
	int keep;
	int cur;
	struct usb *up = USB_BASE;

	keep = up->epr[ep];
	cur = keep & mask;
	keep &= ~EP_TOGGLES;
	up->epr[ep] = keep | val ^ cur;
}

#define ep_rx_stat_write(ep,val) \
	ep_toggle_write ( ep, val, EP_RX_MASK );

#define ep_tx_stat_write(ep,val) \
	ep_toggle_write ( ep, val, EP_TX_MASK );

static void
ep_type_write ( int ep, int type )
{
	int endp;
	struct usb *up = USB_BASE;

	endp = up->epr[ep];
	endp &= EP_TYPE_MASK;
	endp |= type;
	up->epr[ep] = endp;
}

static char *
intcat ( char *p, int val, char *str )
{
	if ( !val )
	    return p;

	while ( *str )
	    *p++ = *str++;
	*p++ = '-';
	return p;
}

static char *
encode_istat ( int stat )
{
	static char buf[64];
	char *p;

	p = buf;
	*p++ = '<';

	if ( stat ) {
	    p = intcat ( p, stat & INT_CTR, "CTR" );
	    p = intcat ( p, stat & INT_OVR, "OVR" );
	    p = intcat ( p, stat & INT_ERR, "ERR" );
	    p = intcat ( p, stat & INT_WKUP, "WKUP" );
	    p = intcat ( p, stat & INT_SUSP, "SUSP" );
	    p = intcat ( p, stat & INT_RESET, "RESET" );
	    p = intcat ( p, stat & INT_SOF, "SOF" );
	    p = intcat ( p, stat & INT_ESOF, "ESOF" );
	    p = intcat ( p, stat & INT_DIR, "DIR" );
	    p--;
	} else {
	    *p++ = '-';
	}

	*p++ = '>';
	*p = '\0';
	return buf;
}

static void
show_int ( char *msg, int stat )
{
	printf ( "- %s: %04x %s\n", msg, stat, encode_istat(stat) );
}

/* Here when we get a USB reset interrupt */
void
usb_reset ( char *msg )
{
	struct usb *up = USB_BASE;
	struct usb_desc *descp;
	int endp;

	printf ( "* Reset: %s\n", msg );

	// printf ( " -- raw\n" );
	// dump_usb_ram ();
	clear_usb_ram ();
	// printf ( " -- cleared\n" );
	// dump_usb_ram ();

	/* This is an offset into USB_RAM */
	up->btable = BTABLE_ADDR;

	descp = &USB_DESC[0];
	descp->rx_addr = ENDP0_RX_ADDR;
	descp->rx_count = RX_BLOCK_32 | (RX_BLOCK_SIZE << RX_BLOCK_SHIFT);
	descp->tx_addr = ENDP0_TX_ADDR;
	// descp->tx_count = USB_MAX_BUF;
	descp->tx_count = 0;

	// printf ( " -- initialized\n" );
	// dump_usb_ram ();

	printf ( "Endpoint reg 0 = %08x\n", up->epr[0] );

	ep_type_write ( 0, EP_TYPE_CONTROL );
	ep_tx_stat_write ( 0, EP_TX_STALL );
	ep_rx_stat_write ( 0, EP_RX_VALID );

	printf ( "Endpoint reg 0 = %08x\n", up->epr[0] );

	up->daddr = DADDR_0 | DADDR_EF;

	pma_show ();
}

// this software only ever uses endpoint 0
//  the other registers are all zeros.
// #define NUM_ENDP     8
#define NUM_ENDP        1

static void
pma_show ( void )
{
        int i, j;
        long *p;
        short *sp;
	struct usb *up = USB_BASE;

        printf ( "BTABLE = %08x\n", up->btable );

        p = USB_RAM;
        sp = (short *) p;

        for ( i=0; i<32; i++ ) {
            printf ( "PMA %08x: ", sp );
            for ( j=0; j<8; j++ ) {
                printf ( "%04x ", *p++ );
                sp++;
            }
            printf ( "\n" );
        }
}

void
usb_init ( void )
{
	struct usb *up = USB_BASE;
	int i;

#ifdef notdef
	/* Try this before turning on USB clocks */
	/* (it worked fine) */
	printf ( " -- raw\n" );
	dump_usb_ram ();
	clear_usb_ram ();
	printf ( " -- cleared\n" );
	dump_usb_ram ();
	printf ( "SPIN\n" );
	for ( ;; )
	    ;
#endif

	/* set up clocks */
	/* de-assert USB module reset */
	rcc_usb_reset ();

	/* clear powerdown and reset
	 * enable some interrupts.
	 */
	up->ctrl = 0;

	for ( i=0; i<NUM_EP; i++ )
	    printf ( "Endp %d: %04x\n", i, up->epr[i] );

	for ( i=0; i<NUM_EP; i++ )
	    up->epr[i] = i;

	for ( i=0; i<NUM_EP; i++ )
	    printf ( "Endp %d: %04x\n", i, up->epr[i] );

	usb_reset ( "initialization" );

	printf ( "USB control: %04x\n", up->ctrl );
	show_int ( "USB status", up->status );

	up->ctrl = CTL_ENABLE;

	printf ( "USB control: %04x\n", up->ctrl );
	show_int ( "USB status", up->status );

	nvic_enable ( USB_HP_IRQ );
	nvic_enable ( USB_LP_IRQ );
	nvic_enable ( USB_WAKEUP_IRQ );

	usb_disconnect ();
}

/* USB uses 3 interrrupt vectors.
 * Here is the "high priority" interrupt
 * Triggered only by a correct transfer event
 *  for isochronous and double-buffer bulk transfer
 *  to reach the highest possible transfer rate.
 * (so ... I never expect to see it).
 */
void
usb_hp_handler ( void )
{
	struct usb *up = USB_BASE;

	show_int ( "USB high priority interrupt", up->status );
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
	    usb_reset ( "interrupt" );
	    last = up->status;
	    // printf ( "USB reset interrupt, %04x\n", up->status );
	    show_int ( "USB reset interrupt", up->status );
	}

	if ( up->status & INT_ERR ) {
	    up->status = ~INT_ERR;
	    last = up->status;
	    // We get these incessantly if no cable is connected.
	    // printf ( "USB error interrupt, %04x\n", up->status );
	} 

	if ( last & INT_ENABLE ) {
	    // printf ( "- USB low priority interrupt: %04x\n", last );
	    show_int ( "USB low priority interrupt", last );
	}
}

/* We clear 2 bytes at a time using 4 byte accesses.
 * There are 512 bytes.
 */
void
clear_usb_ram ( void )
{
	unsigned long *p = USB_RAM;
	int i;

	for ( i=0; i<USB_RAM_SIZE; i += 2 ) {
	    // *p++ = 0x12ab;
	    *p++ = 0;
	}
}

void
dump_usb_ram ( void )
{
	int i, j;
	unsigned long *p = USB_RAM;

	for ( i=0; i<32; i++ ) {
	    for ( j=0; j<8; j++ ) {
		printf ( "%04x ", *p++ );
	    }
	    printf ( "\n" );
	}
}

/* USB wakeup interrupt.
 */
void
usb_wk_handler ( void )
{
	struct usb *up = USB_BASE;

	show_int ( "USB wakeup interrupt", up->status );
}

/* THE END */
