/* usb.c
 *
 * (c) Tom Trebisky  11-5-2023
 *
 * This file is in 3 sections:
 *
 * 1 - definitions and prototypes
 * 2 - essential driver code
 * - - middle section with unsorted code
 * 3 - optional debug and analysis code
 *
 *  IRQ 19 -- USB high priority (or CAN Tx)
 *  IRQ 20 -- USB low priority (or CAN Rx0)
 *  IRQ 42 -- USB wakeup from suspend via EXTI line
 */

#include "protos.h"
#include "usb.h"

volatile enum usb_state usb_state = BOOT;

static int run_test8 = 0;
static void test8_start ( void );
static void test8_ctr ( void );

/* local Prototypes */
void memset ( char *, int, int );
int strlen ( char * );

static void force_reset ( void );
void endpoint_show ( int );
void usb_show ( void );
void enum_logger ( int );
void usb_debug ( void );
static void usb_hw_init ( void );
static void usb_reset ( void );
static void pma_clear ( void );
void usb_set_address ( int );
static void endpoint_init ( void );
void endpoint_recv_ready ( int );
static void pma_copy_in ( u32, char *, int );
static void pma_copy_out ( u32, char *, int );
void enum_log_watch ( void );
static void endpoint_rem ( int );
void print_buf ( char *, int );
static void endpoint_set_rx_ready ( int );
static void endpoint_set_tx_nak ( int );
static void endpoint_clear_rx ( int );
static void endpoint_clear_tx ( int );
static void endpoint_stall ( int );
void endpoint_send_zlp ( int );

static void data_ctr ( void );

#define USB_BASE        (struct usb *) 0x40005C00
#define USB_RAM         (u32 *) 0x40006000

/* An experiment.
 * The addresses are defined in the lds file
 */
struct pma_buf {
	u32 buf[32];
};

/* 4 * 4 bytes = 16 bytes in ARM addr space.
 */
struct btable_entry {
	u32	tx_addr;
	u32	tx_count;
	u32	rx_addr;
	u32	rx_count;
};

extern struct pma_buf PMA_buf[8];
extern struct btable_entry PMA_btable[8];

/* End of experiment */

/* 8 endpoint "pairs"  are allowed */
#define NUM_EP          8

struct usb {
        vu32 epr[NUM_EP];     /* 00 - endpoint registers */
            u32        _pad0[8];
        vu32 ctrl;    /* 40 */
        vu32 isr;     /* 44 - interrupt status */
        vu32 fnr;     /* 48 - frame number */
        vu32 daddr;   /* 4c - device address */
        vu32 btable;  /* 50 - buffer table */
};

/* Interrupt bits (shared by control and isr) */
/* A note on clearing bits in the "isr"
 * The datasheet says not to use read/modify/write
 * Writing 1 is "no change"
 * Writing 0 makes it zero.
 */
#define INT_CTR		BIT(15)
#define INT_PMA		BIT(14)
#define INT_ERR		BIT(13)
#define INT_WKUP	BIT(12)
#define INT_SUSP	BIT(11)
#define INT_RESET	BIT(10)
#define INT_SOF		BIT(9)
#define INT_ESOF	BIT(8)

// #define INT_ALL		0xff00

/* Bits in the Control Register */
#define CTRL_RESUME	BIT(4)
#define CTRL_FSUSP	BIT(3)
#define CTRL_LP_MODE	BIT(2)
#define CTRL_PDWN	BIT(1)
#define CTRL_FRES	BIT(0)

/* Daddr register */
#define DADDR_EN	0x80

/* ================================== */
/* ================================== */

/* Bits in endpoint registers */
#define EP_CTR_RX	BIT(15)
#define EP_DTOG_RX	BIT(14)

/* Hardware sets NAK along with CTR_RX */
/* These bits toggle when a 1 is written
 * Writing 0 does nothing
 */

#define EP_STAT_RX_SHIFT	12
#define EP_STAT_RX		3<<12
#define EP_RX_DIS		0
#define EP_RX_STALL		1<<12
#define EP_RX_NAK		2<<12
#define EP_RX_VALID		3<<12

/* change NAK to VALID via toggle */
#define EP_RX_NAK_VALID		1<<12

#define EP_SETUP	BIT(11)

#define EP_TYPE_BULK		0
#define EP_TYPE_CONTROL		1<<9
#define EP_TYPE_ISO		2<<9
#define EP_TYPE_INTERRUPT	3<<9

/* Depends on Type (modifies type) */
#define EP_KIND		BIT(8)

#define EP_CTR_TX	BIT(7)
#define EP_DTOG_TX	BIT(6)

/* Hardware sets NAK along with CTR_RX */
/* These bits toggle when a 1 is written
 * Writing 0 does nothing
 */
#define EP_STAT_TX_SHIFT	4
#define EP_STAT_TX		3<<4
#define EP_TX_DIS		0
#define EP_TX_STALL		1<<4
#define EP_TX_NAK		2<<4
#define EP_TX_VALID		3<<4

#define EP_ADDR		0xf	// 4 bits

/* =================================== */

#define USB_HP_IRQ	19
#define USB_LP_IRQ	20
#define USB_WK_IRQ	42

/* =================================== */

// #define CONTROL_ENDPOINT	0
// #define DATA_ENDPOINT		1

#define EP_CONTROL	0
#define EP_DATA		1

/* ====================================================== */
/* ====================================================== */
/* section 2 - essential driver code */
/* ====================================================== */

struct endpoint {
	u8	flags;
	u32	*tx_buf;
	u32	*rx_buf;
};

#define	F_RX_BUSY	0x01
#define	F_TX_BUSY	0x02
#define	F_TX_REM	0x80

struct endpoint ep_info[NUM_EP];

void
usb_init ( void )
{
	usb_hw_init ();
	usb_reset ();
	// printf ( "TJT usb init done\n" );
	usb_state = INIT;
}

/* We have 8 endpoint registers.
 * Each handles what I call an "endpoint pair" or what
 * the documentation calls a "bidirectional endpoint".
 *
 * Then we have the BTABLE, which can be located anywhere
 * in PMA ram, as specified by the btable register.
 *
 * Note that BTABLE entries correspond to endpoint registers,
 * not to endpoint index numbers.  For example endpoint
 * register 1 could be set up with the EA field set to 4.
 * This would be endpoint 4, and btable entry 1 would
 * correspond to endoint 4 (via endpoint register 1).
 *
 * Defintely a source of possible confusion.
 *
 * There is more special weirdness
 * Documentation discourages read-modify-write access
 * to these registers and such will give unexpected
 * results.
 *
 * certain bits are type "t" (toggle), meaning if you
 * write 0 to them, nothing happens.  If you write 1,
 * they change state..  DTOG and STAT bits are like this.
 * The CTR bits are type "rc_w0" meaning that you can
 * read them, but can only write 0 (to clear them).
 *
 * In operation, the status bits change between VALID
 * and NAK, so to make status valid again, you don't
 * write 11 as you might think, but you write 01,
 * which changed 10 (nak) to 11 (valid).
 *
 * Sort of crazy, but I didn't design this thing.
 *
 * PMA ram is 512 bytes, but it uses 1024 bytes of ARM address space.
 * The values we write into it are for the USB controller,
 * which lives in the 16 bit world, so it sees 512 bytes
 * at addresses from 0x000 to 0x1ff.
 * Here is the layout:
 *
 * 000 to 01f - btable is here, 4 entries, 8 bytes each
 * 020 to 0f7 - unused.
 * 
 * 0f8 to 137 - 64 bytes, endpoint 1 tx serial data out
 * 138 to 177 - 64 bytes, endpoint 3 Rx serial data in
 * 178 to 17f - 8 bytes, endpoint 2 Tx (ACM)
 * 180 to 1bf - 64 bytes, endpoint 0 Rx
 * 1c0 to 1ff - 64 bytes, endpoint 0 Tx
 *
 * Would I do things this way?  No.
 * I would stick the ACM thing at the start (or get rid of it entirely).
 * I would put serial in/out in the same endpoint number.
 */

/* For the Rx count field in the btable, we leave the 10 low bits
 * for hardware to save the count.  We put the actual buffer size
 * into the upper 6 bits as follows.
 * The high bit (BT_CLICK) declares whether we are using
 * units of 2 bytes or 32 bytes.
 * The next 5 bits give the number of clicks -- MINUS 1.
 * So, 8400 for our 64 byte buffers.  The minus 1 is sneaky.
 */

#define BT_CLICK_32	0x8000		// 0 = 2 byte units, 1 = 32 byte units

// This yields 0x8400 for 64 byte buffers
#define BT_64		(BT_CLICK_32 | 1<<10)

/* Called both during initialization and
 * during a USB reset event.
 */
static void
usb_reset ( void )
{
	endpoint_init ();

	usb_set_address ( 0 );
}

/* Which btable buffer is used for what
 */
#define EP0_TX_BUF_INDEX	1
#define EP0_RX_BUF_INDEX	2
#define EP1_TX_BUF_INDEX	3
#define EP1_RX_BUF_INDEX	4

static void
endpoint_init ( void )
{
        struct usb *up = USB_BASE;
	int ep;

	for ( ep=0; ep < NUM_EP; ep++ )
	    up->epr[ep] = 0;

	up->epr[EP_CONTROL] = EP_TYPE_CONTROL | 0;
	up->epr[EP_DATA] = EP_TYPE_BULK | 1;

	PMA_btable[EP_CONTROL].tx_addr = EP0_TX_BUF_INDEX * 64;
	PMA_btable[EP_CONTROL].tx_count = 0;
	PMA_btable[EP_CONTROL].rx_addr = EP0_RX_BUF_INDEX * 64;
	PMA_btable[EP_CONTROL].rx_count = BT_64;

	endpoint_set_rx_ready ( EP_CONTROL );
	endpoint_set_tx_nak ( EP_CONTROL );

	PMA_btable[EP_DATA].tx_addr = EP1_TX_BUF_INDEX * 64;
	PMA_btable[EP_DATA].tx_count = 0;
	PMA_btable[EP_DATA].rx_addr = EP1_RX_BUF_INDEX * 64;
	PMA_btable[EP_DATA].rx_count = BT_64;

	endpoint_set_rx_ready ( EP_DATA );
	endpoint_set_tx_nak ( EP_DATA );

	ep_info[EP_CONTROL].tx_buf = PMA_buf[1].buf;
	ep_info[EP_CONTROL].rx_buf = PMA_buf[2].buf;
	ep_info[EP_CONTROL].flags = 0;

	ep_info[EP_DATA].tx_buf = PMA_buf[3].buf;
	ep_info[EP_DATA].rx_buf = PMA_buf[4].buf;
	ep_info[EP_DATA].flags = 0;
}

static void
usb_hw_init ( void )
{
        struct usb *up = USB_BASE;

	up->ctrl = 0;
	up->isr = 0;
	up->daddr = 0;
	up->btable = 0;

	/* We only ever see LP interrupts */
	nvic_enable ( USB_HP_IRQ );
	nvic_enable ( USB_LP_IRQ );
	nvic_enable ( USB_WK_IRQ );

	pma_clear ();

	endpoint_init ();

	usb_set_address ( 0 );

	// printf ( "TJT usb init done\n" );
	// printf ( "  ===============\n" );
	// printf ( "  ===============\n" );
	// printf ( "TJT enable ints\n" );

	/* Enable just these two interrupts */
	up->ctrl = INT_CTR | INT_RESET;
}

static int pending_address = 0;

/* Called when we get a SET ADDRESS setup packet.
 * We will save the address and actually
 * set it when the next Tx CTR interrupt happens
 */
void
usb_pend_address ( int addr )
{
	pending_address = addr;
}

void
usb_set_address ( int addr )
{
        struct usb *up = USB_BASE;

	// printf ( "USB address %d set (tjt)\n", addr );
	// printf ( "[%d]\n", addr );

	/* also set stuff in endpoint regs ??
	 * XXX Papoon does, but why?
	 */

	up->daddr = DADDR_EN | (addr & 0x7f);
	up->daddr |= DADDR_EN;

	// Looks correct 40005c4c and 00D3 for address 0x53
	// printf ( "daddr at %08x\n", &up->daddr );
	// printf ( "daddr value: %08x\n", up->daddr );
}

static void
unexpected ( void )
{
	printf ( "********\n" );
	printf ( "********\n" );
	printf ( "********\n" );

	printf ( "Unexpected USB interrupt: %s\n" );

	printf ( "********\n" );
	printf ( "********\n" );
	printf ( "********\n" );

	// spin
	for ( ;; ) ;
}

/* Interrupt handlers,
 * called by vectors in locore.s
 * There are 3 interrupts assigned to USB,
 * but I have only ever seen "lp".
 */
void
usb_hp_handler ( void )
{
	printf ( "USB hp interrupt\n" );
	unexpected ();
}

void
usb_wk_handler ( void )
{
	printf ( "USB wk interrupt\n" );
	unexpected ();
}

static int lp_count = 0;

#ifdef SOF_DEBUG
/* As soon as we plug the cable in, we start getting
 * lots of SOF interrupts, before and during and
 * after enumeration.  The host causes this.
 */
static int sof_count = 0;
#endif

#define SETUP_BUF	10

/* Here when I want to try to handle CTR on endpoint 0
 *
 * So far I have seen one thing outside of enumeration.
 * When I start up picocom, I get this:
 * CTR on endpoint 0 8210
 * EPR[0] = EA60
 * Read 8 bytes from EP 0 2122030000000000
 * The setup bit is set in the EPR
 * This is a class interface request.
 * - not a standard request as in chapter 9 of USB 2.0
 * - it is "control line state"
 */
static void
ctr0 ( void )
{
        struct usb *up = USB_BASE;
	struct btable_entry *bte;
	char buf[SETUP_BUF];
	int count;
	int rv;
	int setup;

	// No interrupts
	// up->ctrl = 0;

	// printf ( "CTR on endpoint %d %04x\n", 0, up->isr );
	// printf ( " EPR[%d] = %04x\n", 0, up->epr[0] );

	if ( up->epr[EP_CONTROL] & EP_CTR_RX ) {

	    /* do we really need this check ? */
	    // bte = & ((struct btable_entry *) USB_RAM) [0];
	    bte = & PMA_btable[EP_CONTROL];
	    count = bte->rx_count & 0x3ff;
	    if ( count > SETUP_BUF ) {
		printf ( "Setup too big: %d\n", count );
		panic ( "setup count" );
	    }

	    /* We need to "grab" this bit before clearing
	     * CTR_RX, as clearling CTR_RX will also clear
	     * SETUP if it is set.
	     */
	    setup = up->epr[EP_CONTROL] & EP_SETUP;

	    endpoint_clear_tx ( EP_CONTROL );
	    count = endpoint_recv ( EP_CONTROL, buf );
	    endpoint_recv_ready ( EP_CONTROL );

	    // printf ( "Read %d bytes from EP 0", count );
	    // print_buf ( buf, count );

	    if ( count == 1 )
		return;

	    if ( setup ) {
		//printf ( "A" );
		rv = usb_setup ( buf, count );
	    } else {
		//printf ( "B" );
		rv = usb_control ( buf, count );
	    }
	    // printf ( "At CTR_RX, epr = %04x\n", up->epr[0] );
	    // return;
	}

	if ( up->epr[EP_CONTROL] & EP_CTR_TX ) {

	    // if ( PMA_btable[0].tx_count == 0 )
	    //	printf ( "z" );

	    endpoint_clear_tx ( EP_CONTROL );
	    // printf ( "At CTR_TX, epr = %04x\n", up->epr[0] );

	    /* Handle the postponed "set address"
	     * This must be the CTR from the ZLP we sent.
	     */
	    if ( pending_address ) {
		// printf ( "a" );
		// printf ( "At pend, epr = %04x\n", up->epr[0] );
		// printf ( "Set pending addr: %d\n", pending_address );
		usb_set_address ( pending_address );
		pending_address = 0;
		// endpoint_stall ( EP_CONTROL );
		return;
	    }

	    /* Send second fragment of Tx packet */
	    if ( ep_info[EP_CONTROL].flags & F_TX_REM ) {
		endpoint_rem ( EP_CONTROL );
		return;
	    }

	    ep_info[EP_CONTROL].flags &= ~F_TX_BUSY;

	    // printf ( "C" );
	    // return usb_control_tx ();
	}

	// return 0;
} // end of ctr0 ();

// int xx_count = 0;

/* Called from interrupt code on any CTR event
 * on a non-zero endpoint
 */
static void
data_ctr ( void )
{
        struct usb *up = USB_BASE;
	struct btable_entry *bte;
	int ep;
	char buf[2];
	int count;

	ep = up->isr & 0xf;

	//if ( xx_count++ < 10 ) {
	//    printf ( "Data CTR on endpoint %d %04x\n", ep, up->isr );
	//    printf ( " EPR[%d] = %04x\n", ep, up->epr[ep] );
	//}

	/* If it ain't 2, it must be 3.
	 * We only ever send data on 3.
	 * We should clear the * CTR bit
	 * either here or when we send new
	 * data (we do the latter).
	 */
	if ( ep != 2 ) {
	    endpoint_clear_tx ( ep );
	    ep_info[EP_DATA].flags &= ~F_TX_BUSY;
	    if ( run_test8 )
		test8_ctr ();
	    return;
	}

#ifdef notdef
	printf ( "Data CTR on endpoint %d %04x\n", ep, up->isr );
	printf ( " EPR[%d] = %04x\n", ep, up->epr[ep] );

	bte = & ((struct btable_entry *) USB_RAM) [ep];
	printf ( " BTE rx_count = %04x\n", bte->rx_count );
#endif

	endpoint_clear_rx ( ep );
	ep_info[EP_DATA].flags &= ~F_RX_BUSY;

	// part of test 8
	// count = endpoint_recv_limit ( ep, buf, 2 );
	// printf ( "Received %d: %c\n", count, buf[0] );

	if ( run_test8 )
	    test8_start ();

	// endpoint_recv_ready ( ep );
}


static int int_count = 0;
static int int_first = 1;

void
usb_lp_handler ( void )
{
        struct usb *up = USB_BASE;
	int ep;

	if ( int_first && int_count++ > 2000 ) {
	    int_first = 0;
	    printf ( "interrupts gone crazy: %04x ep0 = %04x\n", up->isr, up->epr[0] );
	    up->ctrl = 0;
	}

	/* This allows enumeration capture */
	enum_handler ();

	// printf ( "I%04x\n", up->isr );

	/* Reset interrupt.
	 * Yes indeed, the reset interrupt is telling us
	 * that the device itself has been reset and needs
	 * to be fully initialized.
	 */
	if ( up->isr & INT_RESET ) {
	    // printf ( " -- RESET\n" );
	    // usb_show ();
	    //usb_show ();
	    usb_reset ();
	    // printf ( "At RESET, epr = %04x\n", up->epr[0] );
	    up->isr &= ~INT_RESET;
	}

	/* Correct transfer interrupt.
	 * We handle some of these for endpoint 0
	 * We handle all of them for other endpoints
	 */
	if ( up->isr & INT_CTR ) {
	    ep = up->isr & 0xf;
	    // endpoint_show ( ep );
	    // usb_show ();

	    if ( ep == 0 ) {
		ctr0 ();
		// printf ( "M" );
	    } else {
		data_ctr ();
	    }

	    up->isr &= ~INT_CTR;
	}
} // end of usr_lp_handler()

#ifdef SOF_DEBUG
	/* This was an interesting experiment.  Once.
	 * We saw 913 SOF per second.
	 * (actually, 1000 when we got our timer just right)
	 * This confirms that we can clear the INT_SOF bit in the isr
	 * We can also gate off the interrupt by clearing the bit in
	 * the up->ctrl control register.
	 */
	if ( up->isr & INT_SOF ) {
	    /*
	    if ( sof_count > 5000 ) {
		up->ctrl &= ~INT_SOF;
		// up->isr = INT_CTR | INT_RESET;
		printf ( "Last SOF, isr, ctrl = %04x %04x\n", up->isr, up->ctrl );
	    }
	    */

	    // printf ( "SOF int\n" );

	    // everything but INT_SOF
	    // up->isr = INT_CTR | INT_RESET;
	    up->isr = ~INT_SOF;

	    /*
	    if ( sof_count > 5000 )
		printf ( "Last SOF, final isr, ctrl = %04x %04x\n", up->isr, up->ctrl );
	    */

	    sof_count++;
	    return;
	}
#endif

/* PMA (packet memory array) is 512 bytes of SRAM
 * The usb controller sees it as an array of 16 bit words.
 * The ARM sees it as 16 bit words inside 32 bit words.
 * So the ARM address space for PMA covers 1024 bytes.
 * i.e. from 0x40006000 to 63ff
 *
 * The F103 allows the buffers to be 64 bytes maximum.
 * This 512 bytes of PMA sram would allow 8 of those.
 * However the BTABLE must also reside in PMA and it
 * will use up 64 bytes, leaving 7 buffers of size 64.
 * So, if you really wanted to use all 8 endpoints you
 * would need to do something like have two of them be
 * of size 32.
 */
static void
pma_clear ( void )
{
	u32 *p;
	// int i;

        // p = USB_RAM;
	// for ( i=0; i<256; i++ ) 
	//     *p++ = 0;

	// memset ( (char *) USB_RAM, 0xea, 1024 );
	memset ( (char *) USB_RAM, 0x0, 1024 );

        p = USB_RAM;
	p[254] = 0xdead;
	p[255] = 0xdeadbeef;
}

/* Copy from PMA memory to buffer */
static void
pma_copy_in ( u32 pma_off, char *buf, int count )
{
	int i;
	int num = (count + 1) / 2;
	u16 *bp = (u16 *) buf;
	u32 *pp;
	u32 addr;

	addr = (u32) USB_RAM;
	addr += 2 * pma_off;
	pp = (u32 *) addr;

	for ( i=0; i<num; i++ )
	    *bp++ = *pp++;
}

/* Copy from buffer to PMA memory */
static void
pma_copy_out ( u32 pma_off, char *buf, int count )
{
	int i;
	int num = (count + 1) / 2;
	u16 *bp = (u16 *) buf;
	u32 *pp;
	u32 addr;

	addr = (u32) USB_RAM;
	addr += 2 * pma_off;
	pp = (u32 *) addr;

	for ( i=0; i<num; i++ )
	    *pp++ = *bp++;
}

/* Setting the stat field in an Endpoint register requires all
 * kinds of jumping through hoops
 *
 * We have 3 flavor of bits in the endpoint registers.
 * See page 45 in the reference manual for these.
 * we have plain old r/w bits, "toogle" bits and rc_w0 bits.
 *
 * The two CTR bits can be cleared by writing 0, writing 1 is a NOOP.
 *  (these are the "rc_w0 bits")
 * The DTOG, and "status" bits are toggle bits
 *  these "toggle" bits are toggled by writing 1; writing 0 is a NOOP.
 * Then we have plain old r/w bits (EA, type, kind)
 * Finally the SETUP bit is read only, but ...
 *  (but SETUP clears when CTR_RX is cleared)
 */

#define	EP_W0_BITS	(EP_CTR_RX | EP_CTR_TX)
#define	EP_TOGGLE_TX	(EP_DTOG_RX | EP_DTOG_TX | EP_STAT_RX)
#define	EP_TOGGLE_RX	(EP_DTOG_RX | EP_DTOG_TX | EP_STAT_TX)
#define	EP_TOGGLE_ALL	(EP_DTOG_RX | EP_DTOG_TX | EP_STAT_TX | EP_STAT_RX)
#define	EP_TOGGLE_STALL	(EP_DTOG_RX | EP_DTOG_TX)

#ifdef notdef
/* Made obsolete (for now anyway) by the
 * routine below.
 */
static void
endpoint_set_rx ( int ep, int new )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];

	/* Make sure these bits are not changed */
	val |= EP_W0_BITS;
	val &= ~ EP_TOGGLE_RX;

	/* flip these to get desired status */
	val ^= new;

	up->epr[ep] = val;
}
#endif

/* Clear just the Rx CTR bit
 */
static void
endpoint_clear_rx ( int ep )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];

	/* Make sure these bits are not changed */
	// val |= EP_W0_BITS;
	// clear CTR_RX, leave CTR_TX unchanged.
	val &= ~EP_CTR_RX;
	// val |= EP_CTR_TX;
	val &= ~ EP_TOGGLE_ALL;

	up->epr[ep] = val;
}

/* Clear just the Tx CTR bit
 */
static void
endpoint_clear_tx ( int ep )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];

	/* Make sure these bits are not changed */
	// val |= EP_W0_BITS;
	// clear CTR_RX, leave CTR_TX unchanged.
	val &= ~EP_CTR_TX;
	val |= EP_CTR_RX;
	// val &= ~ EP_TOGGLE_RX;
	val &= ~ EP_TOGGLE_ALL;

	up->epr[ep] = val;
}

/* Same as the above, but always marks status
 * as VALID, and it clears the CTR_RX bit
 *
 * Note: this will also clear the SETUP bit if
 *  it was set.  The SETUP bit is read only,
 *  but it is "released" when CTR_RX is cleared.
 */
static void
endpoint_set_rx_ready ( int ep )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];

	/* Make sure these bits are not changed */
	// val |= EP_W0_BITS;
	// clear CTR_RX, leave CTR_TX unchanged.
	val &= ~EP_CTR_RX;
	val |= EP_CTR_TX;
	val &= ~ EP_TOGGLE_RX;

	/* flip these to get desired status */
	val ^= EP_RX_VALID;

	up->epr[ep] = val;
}

static void
endpoint_stall ( int ep )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];

	/* Make sure these bits are not changed */
	// val |= EP_W0_BITS;
	// clear both CTR_RX and CTR_TX
	val &= ~EP_CTR_RX | EP_CTR_TX;
	// val &= ~ EP_TOGGLE_STALL;
	val &= ~ EP_TOGGLE_RX;

	/* flip these to get desired status */
	// val ^= EP_RX_STALL | EP_TX_STALL;;
	val ^= EP_TX_STALL;;

	up->epr[ep] = val;
}

#ifdef notdef
/* Made obsolete (for now anyway) by the
 * routine below.
 */
static void
endpoint_set_tx ( int ep, int new )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];
	// printf ( "Set tx in: %04x\n", val );

	/* Make sure these bits are not changed */
	val |= EP_W0_BITS;
	val &= ~ EP_TOGGLE_TX;

	/* flip these to get desired status */
	val ^= new;

	up->epr[ep] = val;
	// printf ( "Set tx out: %04x --> %04x\n", val, up->epr[ep] );
}
#endif

/* Called when we send.
 * we set status VALID and also clear
 * the CTR_TX bit.
 */
static void
endpoint_set_tx_valid ( int ep )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];
	// printf ( "Set tx in: %04x\n", val );

	/* Make sure these bits are not changed */
	// clear CTR_TX, leave CTR_RX unchanged.
	val &= ~EP_CTR_TX;
	val |= EP_CTR_RX;
	val &= ~ EP_TOGGLE_TX;

	/* flip these to get desired status */
	val ^= EP_TX_VALID;

	up->epr[ep] = val;
	// printf ( "Set tx out: %04x --> %04x\n", val, up->epr[ep] );
}

static void
endpoint_set_tx_nak ( int ep )
{
        struct usb *up = USB_BASE;
	u32 val;

	val = up->epr[ep];
	// printf ( "Set tx in: %04x\n", val );

	/* Make sure these bits are not changed */
	// clear CTR_TX, leave CTR_RX unchanged.
	val &= ~EP_CTR_TX;
	val |= EP_CTR_RX;
	val &= ~ EP_TOGGLE_TX;

	/* flip these to get desired status */
	val ^= EP_TX_NAK;

	up->epr[ep] = val;
	// printf ( "Set tx out: %04x --> %04x\n", val, up->epr[ep] );
}

#define ENDPOINT_LIMIT	64

/* This crazy business is only set up to work on endpoint 0
 */
static char *tx_rem;
static int tx_rem_count;

/* We can clear the flag as soon as we have copied into
 * PMA memory.
 */
static void
endpoint_rem ( int ep )
{
	struct btable_entry *bte;

	// printf ( "%" );
	bte = & ((struct btable_entry *) USB_RAM) [ep];

	bte->tx_count = tx_rem_count;
	pma_copy_out ( bte->tx_addr, tx_rem, tx_rem_count );
	endpoint_set_tx_valid ( ep );

	ep_info[ep].flags &= ~F_TX_REM;
}

/* send data on an endpoint */
/* Allow for more data than can be sent in a single transaction.
 * This requires a wonky arangement with a "callback" in the
 * interrupt handler -- and only works if the total length
 * will fit in 2 transaactions (currently anyway).
 * The need for this is to support the 67 byte config
 * packet that we send during enumeration.
 * If I figure out how to break up that packet, I will do
 * away with all of this.
 *
 * This cannot just return while this is pending since the
 * remnant data is (very likely) on the stack.
 * It actually isn't in the case of the config packet
 * in question (which is const and in flash), but coding
 * it this way protects us for the future, sort of.
 */
void
endpoint_send ( int ep, char *buf, int count )
{
	struct btable_entry *bte;

	bte = & ((struct btable_entry *) USB_RAM) [ep];

	if ( count <= ENDPOINT_LIMIT ) {
	    bte->tx_count = count;
	    pma_copy_out ( bte->tx_addr, buf, count );
	    endpoint_set_tx_valid ( ep );
	    return;
	}

	tx_rem = &buf[ENDPOINT_LIMIT];
	tx_rem_count = count - ENDPOINT_LIMIT;
	ep_info[ep].flags |= F_TX_REM;

	bte->tx_count = ENDPOINT_LIMIT;
	pma_copy_out ( bte->tx_addr, buf, ENDPOINT_LIMIT );
	endpoint_set_tx_valid ( ep );

	// NO!  Waiting here won't work.  We are already in an
	//  interrupt handler during enumeration, but it is OK
	//  if our strings are const and in flash and not
	//  on the stack.
	//while ( ep_flags[ep] & F_TX_REM )
	//    ;
}

#ifdef notdef
/* send data on an endpoint */
/* This is the simple version before we added the rubbish
 * to allow more than 64 bytes per call (above)
 */
void
endpoint_send ( int ep, char *buf, int count )
{
	struct btable_entry *bte;


	bte = & ((struct btable_entry *) USB_RAM) [ep];

	bte->tx_count = count;

	pma_copy_out ( bte->tx_addr, buf, count );

	// endpoint_set_tx ( ep, EP_TX_VALID );
	endpoint_set_tx_valid ( ep );

	// endpoint_show ( ep );
}
#endif

/* Send a zero length packet */
void
endpoint_send_zlp ( int ep )
{
	struct btable_entry *bte;
        struct usb *up = USB_BASE;

	// printf ( "Z" );

	bte = & ((struct btable_entry *) USB_RAM) [ep];

	bte->tx_count = 0;

	//printf ( "zlp EPR[%d] = %04x\n", ep, up->epr[ep] );
	endpoint_set_tx_valid ( ep );
	// printf ( "zlp EPR[%d] = %04x\n", ep, up->epr[ep] );
}


/* get data that has been received on an endpoint */
int
endpoint_recv ( int ep, char *buf )
{
	struct btable_entry *bte;
	int count;

	bte = & ((struct btable_entry *) USB_RAM) [ep];

	count = bte->rx_count & 0x3ff;
	pma_copy_in ( bte->rx_addr, buf, count );

	return count;
}

/* get data that has been received on an endpoint */
int
endpoint_recv_limit ( int ep, char *buf, int limit )
{
	struct btable_entry *bte;
	int count;

	bte = & ((struct btable_entry *) USB_RAM) [ep];

	count = bte->rx_count & 0x3ff;
	if ( count > limit ) count = limit;

	pma_copy_in ( bte->rx_addr, buf, count );

	return count;
}

/* flag an endpoint ready to receive */
void
endpoint_recv_ready ( int ep )
{
	struct btable_entry *bte;

	bte = & ((struct btable_entry *) USB_RAM) [ep];

	bte->rx_count &= ~0x3ff;

	// endpoint_set_rx ( ep, EP_RX_VALID );
	endpoint_set_rx_ready ( ep );
}

#ifdef notdef
/* Not useful */
static void
force_reset ( void )
{
        struct usb *up = USB_BASE;

	printf ( "Forcing USB reset\n" );

	up->ctrl = CTRL_CTRM | CTRL_RESETM | CTRL_FRES;
	// delay_ms ( 100 );
	delay_ms ( 10 );
	up->ctrl = CTRL_CTRM | CTRL_RESETM;
}
#endif

/* ====================================================== */
/* ====================================================== */
/* section XXX - odds and ends */
/* ====================================================== */

#ifdef notdef
/* Tested 11-7-2023 */
void
usb_putc ( int cc )
{
}

static int puts_count = 0;

#ifdef not_ever
void
usb_puts ( char *msg )
{
	puts_count++;
	printf ( "%5d - Send %d chars: %s\n", puts_count, strlen(msg), msg );
	xyz_send ( msg, strlen(msg) );
}
#endif

void
usb_puts ( char *msg )
{
	// puts_count++;
	// printf ( "%5d - Send %d chars: %s\n", puts_count, strlen(msg), msg );
	while ( *msg )
	    usb__putc ( *msg++ );
}
#endif

/* ====================================================== */
/* ====================================================== */
/* section 3 - analysis and debug code */
/* ====================================================== */

static void test1 ( void );
static void test6 ( void );
static void test7 ( void );
static void test10 ( void );

void
enum_wait ( void )
{
	while ( usb_state != CONFIGURED )
	    ;
}

#ifdef notdef
void
enum_wait ( void )
{
	int ticks = 5;

	while ( ticks-- )
	    delay_ms ( 1000 );
}
#endif

void
usb_debug ( void )
{
	// usb_show ();

	enum_log_init ();

	//printf ( "USB spins\n" );
	//for ( ;; ) ;

	//usb_yank ();

	// btable_show ();
	// pma_show ();

	// run echo demo
	// test1 ();
	// test2 ();
	// test3 ();
	// test4 ();

	// run enumeration detailer
	// enum_log_watch ();
	enum_wait ();
	enum_log_show ();

#ifdef notdef
	delay_sec ( 4 );
	printf ( "USB Disconnect\n" );
	usb_disconnect ();
	enum_log_watch ();
#endif

	// the papooon echo demo
	// test1 ();

	// test6 ();
	// test7 ();

	// test 8 and 9 just spin while
	// all the action is driven by
	// interrupts

	// usb_show ();

	/* echo test */
	test10 ();
}

#ifdef SOF_DEBUG
/* I see 913 SOF per second.
 * My delay_ms may not be precise, so this is an estimate.
 * -- indeed the delays were crude, we get 1000 SOF per second
 *
 * We still see the SOF bit getting set in the isr register
 * even after we turn it off in the ctrl register.
 * This confirms that interrupt events do get posted in
 * the isr register, even if they are not enabled, hence
 * we could poll for and clear them if desired.
 */
static void
test4 ( void )
{
	int last = 0;
	int cur;
	int num;
        struct usb *up = USB_BASE;

	for ( ;; ) {
	    delay_ms ( 1000 );
	    cur = sof_count;
	    num = cur-last;
	    last = cur;
	    // if ( num )
		printf ( "SOF count = %d, %d %04x\n", num, cur, up->isr );
	}
}
#endif

/* Works beautifully sending single characters.
 * try sending 2 and you get nothing at the other end.
 * Perhaps this is part of the ACM specification?
 *
 * At any event, this seems to demonstrate that
 *  endpoint_send() works.
 */
static void
test6 ( void )
{
	char buf[4];

	buf[0] = '7';
	// buf[1] = '8';

	for ( ;; ) {
	    if ( buf[0] == '7' )
		buf[0] = '-';
	    else
		buf[0] = '7';
	    endpoint_send ( 3, buf, 1 );
	    // endpoint_send ( 3, buf, 2 );
	    delay_ms ( 5 );
	}
}

/* This fails
 * Wireshark shows a 2 byte packet getting sent,
 * so it is not my code failing to send data,
 * but some issue with picocom on the other end and/or
 * the linux ACM driver.
 */
static void
test7 ( void )
{
	char buf[4];

	buf[0] = '7';
	buf[1] = '8';

	for ( ;; ) {
	    endpoint_send ( 3, buf, 2 );
	    delay_ms ( 5 );
	}
}

/* The object here is to send characters as fast as we can
 * back to back.
 * It gets help from code in the interrupt handler.
 *
 * We see about 7 seconds for 500,000 characters,
 * so 71,428 characters per second
 * 71 characters per millisecond.
 */

#define TEST8_LIMIT	500000

int test8_count;
int test8_run = 0;

static void
test8_send ( void )
{
	char buf[2];

	buf[0] = '5';

	endpoint_send ( 3, buf, 1 );
}

static void
test8_start ( void )
{
	test8_count = 0;
	test8_run = 1;

	printf ( "Test 8 start\n" );

	test8_send ();
}

static void
test8_ctr ( void )
{
	if ( ! test8_run )
	    return;

	if ( test8_count > TEST8_LIMIT ) {
	    printf ( "Test 8 done\n" );
	    test8_run = 0;
	    return;
	}

	++test8_count;

	test8_send ();
}

static void
test8 ( void )
{
	run_test8 = 1;
}

int
ep_recv ( int ep, char *buf, int limit )
{
	int rv;

	while ( ep_info[ep].flags & F_RX_BUSY )
	    ;

	rv = endpoint_recv_limit ( ep, buf, limit );

	endpoint_recv_ready ( ep );
	ep_info[ep].flags |= F_RX_BUSY;

	return rv;
}

void
ep_send ( int ep, char *buf, int count )
{
	while ( ep_info[ep].flags & F_TX_BUSY )
		;

	endpoint_send ( ep, buf, count );

	ep_info[ep].flags |= F_TX_BUSY;
}

static void
test10 ( void )
{
	char buf[2];
	int count;
	int echo_count = 0;

	printf ( "Running echo demo (test10)\n" );

	// XXX - should be done in an init routine
	endpoint_recv_ready ( EP_DATA );
	ep_info[EP_DATA].flags |= F_RX_BUSY;

	for ( ;; ) {

	    printf ( "Waiting for data on endpoint %d\n", EP_DATA );
	    count = ep_recv ( EP_DATA, buf, 2 );

	    printf ( "Got data: %d\n", count );

	    if ( count < 1 )
		continue;

	    if ( count > 1 )
		printf ( "Surprise, recevied %d characters\n", count );

	    if ( buf[0] == '?' )
		printf ( "%d characters echoed so far\n", echo_count );
	    echo_count++;

	    //printf ( "Waiting for xmit\n" );
	    ep_send ( EP_DATA, buf, count );

	}
}

void
ep_btable_show ( int ep )
{
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
	struct btable_entry *bte;
	char *buf;

	bte = &bt[ep];
	printf ( "BTE ep-%d @ %08x\n", ep, bte );
	buf = (char *) USB_RAM + 2 * bte->tx_addr;
	printf ( "BTE, ep-%d tx addr  = %04x - %08x\n", ep, bte->tx_addr, buf );
	printf ( "BTE, ep-%d tx count = %04x\n", ep, bte->tx_count );

	buf = (char *) USB_RAM + 2 * bte->rx_addr;
	printf ( "BTE, ep-%d rx addr  = %04x - %08x\n", ep, bte->rx_addr, buf );
	printf ( "BTE, ep-%d rx count = %04x\n", ep, bte->rx_count );
}

/* Display stuff for a specific endpoint pair
 */
void
endpoint_show ( int ep )
{
        struct usb *up = USB_BASE;

	if ( up->epr[ep] == 0 )
	    return;

	printf ( "Endpoint %d: EPR = %04x\n", ep, up->epr[ep] );
	ep_btable_show ( ep );

}

/* Display entire btable and all endpoints
 */
void
btable_show ( void )
{
        struct usb *up = USB_BASE;
	int ep;

	if ( up->btable ) {
	    printf ( "BTABLE = %08x\n", up->btable );
	    printf ( "Btable non-zero, giving up\n" );
	    return;
	}

	for ( ep=0; ep<8; ep++ ) {
	    endpoint_show ( ep );
	}
}

void
usb_show ( void )
{
        struct usb *up = USB_BASE;

        printf ( "BTABLE = %04x\n", up->btable );
        printf ( "CTRL  = %04x\n", up->ctrl );
        printf ( "ISR   = %04x\n", up->isr );
        printf ( "EPR-0 = %04x\n", up->epr[0] );
        printf ( "EPR-1 = %04x\n", up->epr[1] );
        printf ( "EPR-2 = %04x\n", up->epr[2] );
        printf ( "EPR-3 = %04x\n", up->epr[3] );

	btable_show ();
	ep_btable_show ( 0 );
}

/* Dump the above AND the entire PMA area.
 */
void
pma_show ( void )
{
        int i, j;
        u32 *p;
        u16 *addr;
        // struct usb *up = USB_BASE;

	usb_show ();

        p = USB_RAM;
        addr = (short *) 0;

        for ( i=0; i<32; i++ ) {
            printf ( "PMA %08x: ", addr );
            for ( j=0; j<8; j++ ) {
                printf ( "%04x ", *p++ );
                addr++;
            }
            printf ( "\n" );
        }
}

void
print_buf ( char *data, int count )
{
	int i;

	if ( count ) {
	    printf ( " " );
	    for ( i=0; i<count; i++ )
		printf ( "%02x", *data++ );
	}

	printf ( "\n" );
}

/* THE END */
