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
 * Currently this hands off most of the work to papoon.
 * look for baboon.cxx in the papoon directory.
 *
 * More and more is being done here by this C code.
 *
 *  IRQ 19 -- USB high priority (or CAN Tx)
 *  IRQ 20 -- USB low priority (or CAN Rx0)
 *  IRQ 42 -- USB wakeup from suspend via EXTI line
 */

#include "protos.h"

// set to 0 to let papoon handle CTR
// int my_ctr = 1;
int my_ctr = 0;

// typedef unsigned short u16;
// typedef unsigned int u32;
// typedef volatile unsigned int vu32;
// #define BIT(x)	(1<<x)

/* local Prototypes */
static void memset ( char *, int, int );
static void force_reset ( void );
void endpoint_show ( int );
void usb_show ( void );
void tjt_enum_logger ( int );
void usb_debug ( void );
static void usb_hw_init ( void );
static void pma_clear ( void );
static void set_address ( int );
static void endpoint_init ( void );
static void pma_copy_in ( u32, char *, int );
static void pma_copy_out ( u32, char *, int );
void enum_log_watch ( void );

#define USB_BASE        (struct usb *) 0x40005C00
#define USB_RAM         (u32 *) 0x40006000

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

/* 4 * 4 bytes = 16 bytes in ARM addr space.
 */
struct btable_entry {
	u32	tx_addr;
	u32	tx_count;
	u32	rx_addr;
	u32	rx_count;
};

/* =================================== */

#define USB_HP_IRQ	19
#define USB_LP_IRQ	20
#define USB_WK_IRQ	42

/* ====================================================== */
/* ====================================================== */
/* section 2 - essential driver code */
/* ====================================================== */

void
usb_init ( void )
{
	usb_hw_init ();

	usb_debug ();
}

static void
usb_hw_init ( void )
{
        struct usb *up = USB_BASE;

	// No interrupts
	up->ctrl = 0;

	/* We only ever see LP interrupts */
	nvic_enable ( USB_HP_IRQ );
	nvic_enable ( USB_LP_IRQ );
	nvic_enable ( USB_WK_IRQ );

	pma_clear ();

	endpoint_init ();

	set_address ( 0 );

	/* Clear all interrupts */
	// up->isr = INT_ALL;
	up->isr = 0;

	printf ( "TJT usb init done\n" );
	printf ( "  ===============\n" );
	printf ( "  ===============\n" );
	printf ( "TJT enable ints\n" );

	/* Enable just these two */
	up->ctrl = INT_CTR | INT_RESET;
}

/* XXX - could be inline or macro or
 * just placed inline by hand
 */
static void
set_address ( int addr )
{
        struct usb *up = USB_BASE;

	printf ( "USB address %d set (tjt)\n", addr );

	/* also set stuff in endpoint regs ??
	 * XXX see above, why?
	 */

	up->daddr = DADDR_EN | (addr & 0x7f);
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

static void
endpoint_init ( void )
{
        struct usb *up = USB_BASE;
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
        struct btable_entry *bte;
	int i;

	for ( i=0; i<NUM_EP; i++ )
	    up->epr[i] = 0;

	/* This is how papoon does things,
	 * and I will stick with it for now.
	 */
	up->epr[0] = EP_TYPE_CONTROL | 0;
	up->epr[1] = EP_TYPE_INTERRUPT | 2;  // ?? for ACM
	up->epr[2] = EP_TYPE_BULK | 3;
	up->epr[3] = EP_TYPE_BULK | 1;

	// Endpoint 0
	bte = &bt[0];
	bte->tx_addr = 0x180;
	// bte->tx_count = 64;
	bte->tx_count = 0;
	bte->rx_addr = 0x1c0;
	bte->rx_count = BT_64;

	// I never see traffic here, but it is
	// apparently "required" for an ACM device.
	// Endpoint 2 - ACM control (8 bytes)
	bte = &bt[1];
	bte->tx_addr = 0x178;
	bte->tx_count = 0;

	// Endpoint 3 - serial data in
	bte = &bt[2];
	bte->tx_addr = 0;
	bte->tx_count = 0;;
	bte->rx_addr = 0x138;
	bte->rx_count = BT_64;

	// Endpoint 1 - serial data out
	bte = &bt[3];
	bte->tx_addr = 0x0f8;
	bte->tx_count = 0;

	up->btable = 0;

	// We don't set EP_KIND
	// This will work to transition from 00 (disable) to 10 (NAK)
	up->epr[0] = EP_RX_NAK | EP_TX_NAK;

	/* Toggle NAK to VALID */
	up->epr[0] = EP_RX_NAK_VALID;
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

/* Having debug printout during enumeration
 * caused the enumeration to fail.
 * The debug switch was introduced with the
 * idea of disabling debug during enumeration,
 * then enabling it later.
 * (However most of what we are interested in
 *  is what happens during initialization, so
 *  we tend to either turn it off entirely,
 *  or enable it, allowing us to learn things,
 *  but being fully aware that enumeration
 *  will fail.
 */

/* The idea here is to capture enumeration activity
 * for later analysis without disturbing anything.
 * (i.e. a passive watcher)
 */
void
interrupt_debug ( void )
{
        struct usb *up = USB_BASE;
	int ep;

	if ( up->isr & INT_RESET )
	    tjt_enum_logger ( 2 );

	if ( up->isr & INT_CTR ) {
	    ep = up->isr & 0xf;
	    if ( ep == 0 ) {
		if ( up->epr[0] & EP_CTR_RX )
		    tjt_enum_logger ( 0 );
		if ( up->epr[0] & EP_CTR_TX )
		    tjt_enum_logger ( 1 );
	    }
	}
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

static int
ctr0 ( void )
{
        struct usb *up = USB_BASE;
	struct btable_entry *bte;
	char buf[SETUP_BUF];
	int count;

	// No interrupts
	// up->ctrl = 0;

	if ( up->epr[0] & EP_CTR_RX ) {

	    /* do we really need this check ? */
	    bte = & ((struct btable_entry *) USB_RAM) [0];
	    count = bte->rx_count & 0x3ff;
	    if ( count > SETUP_BUF ) {
		printf ( "Setup too big: %d\n", count );
		panic ( "setup count" );
	    }

	    count = endpoint_recv ( 0, buf );

	    if ( up->epr[0] & EP_SETUP )
		return usb_setup ( buf, count );
	    else
		return usb_control ( buf, count );
#ifdef notdef
#endif
	}

	if ( up->epr[0] & EP_CTR_TX ) {
	    return usb_control_tx ();
	}

	return 0;
}

static int
dummy_ctr0 ( void )
{
	return 0;
}

void
usb_lp_handler ( void )
{
        struct usb *up = USB_BASE;
	int ep;
	int stat;

	/* This allows enumeration capture */
	interrupt_debug ();

	/* for now, relay to papoon */
	if ( up->isr & INT_RESET ) {
	    P_reset ();
	    up->isr &= ~INT_RESET;
	}

	if ( up->isr & INT_CTR ) {
	    ep = up->isr & 0xf;
	    stat = 0;
	    if ( ep == 0 ) {
		// endpoint_show ( 0 );
		if ( my_ctr )
		    stat = ctr0 ();
		else
		    stat = dummy_ctr0 ();
	    }
	    if ( stat ) {
		/* XXX - not really a panic, but we
		 * stop the show for debugging.
		 */
		panic ( "bad CTR\n" );
		up->isr &= ~INT_CTR;
	    }
	    if ( ep != 0 ) {
		// endpoint_show ( ep );
		// usb_show ();
	    }
	}

#ifdef SOF_DEBUG
	/* We see 913 SOF per second.
	 * (actually, 1000 when we get our timer just right)
	 * This confirms that we can clear the INT_SOF bit in the isr
	 * We also gate off the interrupt by clearing the bit in CTR.
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

	P_handler ();
}

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
 * The two CTR bits can be cleared by writing 0, writing 1 is a NOOP.
 * The various "toggle" bits are toggled by writing 1, 0 is a NOOP.
 * The DTOG, and "status" bits are toggle bits
 * Then we have plain old r/w bits (EA, type, kind)
 * Finally the SETUP bit is read only
 */

#define	EP_W0_BITS	(EP_CTR_RX | EP_CTR_TX)
#define	EP_TOGGLE_TX	(EP_DTOG_RX | EP_DTOG_TX | EP_STAT_RX)
#define	EP_TOGGLE_RX	(EP_DTOG_RX | EP_DTOG_TX | EP_STAT_TX)

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

/* send data on an endpoint */
void
endpoint_send ( int ep, char *buf, int count )
{
	struct btable_entry *bte;


	bte = & ((struct btable_entry *) USB_RAM) [ep];

	bte->tx_count = count;

	pma_copy_out ( bte->tx_addr, buf, count );
	endpoint_set_tx ( ep, EP_TX_VALID );
	// endpoint_show ( ep );
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

/* flag an endpoint ready to receive */
void
endpoint_recv_ready ( int ep )
{
	struct btable_entry *bte;

	bte = & ((struct btable_entry *) USB_RAM) [ep];

	bte->rx_count &= ~0x3ff;

	endpoint_set_rx ( ep, EP_RX_VALID );
}

/* Quick and dirty.
 * could be in a library somewhere, but it isn't
 */
static void
memset ( char *buf, int val, int count )
{
	char *p = buf;
	int i;

	for ( i=0; i<count; i++ )
	    *p++ = val;
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

static int
strlen ( char *x )
{
	int rv = 0;

	while ( *x++ )
	    rv++;

	return rv;
}

/* Tested 11-7-2023 */
void
usb_putc ( int cc )
{
	papoon_putc ( cc );
}

static int puts_count = 0;

#ifdef notdef
void
usb_puts ( char *msg )
{
	puts_count++;
	printf ( "%5d - Send %d chars: %s\n", puts_count, strlen(msg), msg );
	papoon_send ( msg, strlen(msg) );
}
#endif

void
usb_puts ( char *msg )
{
	// puts_count++;
	// printf ( "%5d - Send %d chars: %s\n", puts_count, strlen(msg), msg );
	while ( *msg )
	    papoon_putc ( *msg++ );
}

/* ===================================================== */

/* Papoon code (that will go away someday?)
 * mostly below here.
 */

/* This is what I use to manage an endpoint pair
 */
struct endpoint {
	struct btable_entry *btable;
	u32 *tx_buf;
	u32 *rx_buf;
	int busy;
};

struct endpoint endpoint_info[4];

/* I only use endpoint 0 (for control stuff)
 * and endpoint 1 (for the ACM)
 */
static void
endpoint_info_init ( void )
{
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
	struct btable_entry *bte;
	struct endpoint *epp;

	/* We are just picking up the setup done by papoon
	 */

	epp = &endpoint_info[0];
	bte = &bt[0];
	epp->btable = bte;
	epp->tx_buf = (u32 *) ((char *) USB_RAM + 2 * bte->tx_addr);
	epp->rx_buf = (u32 *) ((char *) USB_RAM + 2 * bte->rx_addr);

	epp = &endpoint_info[1];
	bte = &bt[1];
	epp->btable = bte;
	epp->tx_buf = (u32 *) ((char *) USB_RAM + 2 * bte->tx_addr);
	epp->rx_buf = (u32 *) ((char *) USB_RAM + 2 * bte->rx_addr);
}

/* Send a single character on endpoint 1 */
void
tom_putc ( int cc )
{
	struct endpoint *epp = &endpoint_info[1];
        struct usb *up = USB_BASE;

	// XXX spin on busy

	*(epp->tx_buf) = 0x3132;	// "12"
	epp->btable->tx_count = 2;
	epp->busy = 1;

	// up->epr[2] = xxx;
}

/* ====================================================== */
/* ====================================================== */
/* section 3 - analysis and debug code */
/* ====================================================== */

static void test1 ( void );
static void test6 ( void );

/*
 * Starting this up is a bit problematic.
 * 1 - if we just reboot the code with the USB cable
 *   plugged in, the linux system doesn't know anything
 *   happened, so doesn't start enumeration.
 *   So papoon_init() just times out.
 * 2 - if we boot it up, let the above time out,
 *   then unplug and replug the cable, we do get
 *   the linux system trying to enumerate, but we
 *   are running in papoon_demo() and our debug
 *   kills the enumeration.
 */
void
usb_debug ( void )
{
	// usb_show ();

	// papoon_debug ();

	papoon_init ();

	//printf ( "USB spins\n" );
	//for ( ;; ) ;

	//usb_yank ();

	// fairly useless
	// papoon_wait ( 10 );

	// btable_show ();
	// pma_show ();

	// papoon_debug ();

	// run echo demo
	// test1 ();

	// test2 ();
	// test3 ();
	// test4 ();

	// papoon_debug ();

	// run enumeration detailer
	enum_log_watch ();

#ifdef notdef
	delay_sec ( 4 );
	printf ( "USB Disconnect\n" );
	usb_disconnect ();
	enum_log_watch ();
#endif

	// the papooon echo demo
	// test1 ();

	test6 ();
}

/* This is the original test that came with papoon.
 * It echos characters on the ACM serial port.
 * Note that the way picocom works we only ever
 * receive (and then echo) once character at a
 * time with considerable delay between
 * characters.
 */
static void
test1 ( void )
{
	papoon_demo ();
}

/* This is my test that spits out "5" endlessly to
 * test my addition of a putc function.
 * BUG!  We see all the 5 OK, but never see a 6.
 * The delay seems vital.
 */
static void
test2 ( void )
{
	for ( ;; ) {
	    if ( is_papoon_configured () )
		papoon_putc ( '5' );
	    // adding this delay makes it work.
	    // without it, we never see the '6'
	    // delay_ms ( 1 );
	    if ( is_papoon_configured () )
		papoon_putc ( '6' );
	    delay_ms ( 100 );
	}
}

/* This is my test with endless puts messages.
 *  to test my addition of a puts function.
 * It mostly does NOT work.  It worked once, but
 * I get erratic results and they depend on what
 * state picocom is on the other end, or something
 * of the sort.
 * It is odd that this fails even when I implement
 * usb_puts() by looping on usb_putc()
 */
static void
test3 ( void )
{
	char buf[40];
	int count = 0;

	for ( ;; ) {
	    if ( is_papoon_configured () ) {
		count++;
		snprintf ( buf, 40, "Happy day: %d\n\r", count );
		usb_puts ( buf );
	    }
	    delay_ms ( 100 );
	}
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


/* Display stuff for a specific endpoint pair
 */
void
endpoint_show ( int ep )
{
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
	struct btable_entry *bte;
        struct usb *up = USB_BASE;
	char *buf;

	if ( up->epr[ep] == 0 )
	    return;

	printf ( "Endpoint %d: EPR = %04x\n", ep, up->epr[ep] );

	bte = &bt[ep];
	printf ( "BTE ep-%d @ %08x\n", ep, bte );
	buf = (char *) USB_RAM + 2 * bte->tx_addr;
	printf ( "BTE, ep-%d tx addr  = %04x - %08x\n", ep, bte->tx_addr, buf );
	printf ( "BTE, ep-%d tx count = %04x\n", ep, bte->tx_count );

	buf = (char *) USB_RAM + 2 * bte->rx_addr;
	printf ( "BTE, ep-%d rx addr  = %04x - %08x\n", ep, bte->rx_addr, buf );
	printf ( "BTE, ep-%d rx count = %04x\n", ep, bte->rx_count );
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

/* ***************************************** */
/* ***************************************** */
/* Enumeration logging facility */
/* ***************************************** */

static char * enum_saver ( u32, int );
void print_buf ( char *, int );

/* Called from papoon on CTR interrupts on
 * endpoint 0
 * what = 0 is Rx, 1 is Tx
 */

struct enum_log_e {
	int what;
	int count;
	// u32 addr;
	u32 istr;
	u32 epr;
	char *data;
};

/* We see 36 entries */
static struct enum_log_e e_log[50];
static int e_count = 0;

/* We see 363 bytes */
#define SAVE_SIZE	400

static char save_buf[SAVE_SIZE];
static int s_count = 0;

void
tjt_enum_logger ( int what )
{
	struct enum_log_e *ep;
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
	struct btable_entry *bte;
	static int first = 0;

        struct usb *up = USB_BASE;
	u32 addr = (u32) USB_RAM;

#ifdef notdef
	if ( first && what != 2 )
	    pma_show ();
#endif

	bte = &bt[0];

	// Even printing this breaks enumeration.
	// printf ( "enum %d\n", what );

	ep = &e_log[e_count];
	ep->what = what;
	ep->count = 0;
	ep->istr = up->isr;
	ep->epr = up->epr[0];

	if ( what == 1 ) {	// Tx
	    ep->count = bte->tx_count;
	    //ep->addr = addr + 2 * bte->tx_addr;
	    ep->data = enum_saver ( bte->tx_addr, bte->tx_count );
	}

	if ( what == 0 ) {	// Rx
	    ep->count = bte->rx_count & 0x3ff;
	    // ep->addr = addr + 2 * bte->rx_addr;
	    ep->data = enum_saver ( bte->rx_addr, bte->rx_count );
	}

	if ( first && what != 2 ) {
	    printf ( "buf (%d): ", ep->count );
	    print_buf ( ep->data, ep->count );
	    printf ( "\n" );
	    first = 0;
	}

	e_count++;
}

void
enum_log_show ( void )
{
	int i;
	struct enum_log_e *ep;
	char *wstr;
	char *sstr;

	// printf ( "Log entries: %d\n", e_count );

	for ( i=0; i<e_count; i++ ) {
	    ep = &e_log[i];
	    if ( ep->what == 2 ) {
		printf ( "%3d Enum Reset\n", i );
		continue;
	    }

	    wstr = ep->what == 1 ? "Tx" : "Rx";
	    sstr = ep->epr & EP_SETUP ? "S" : " ";

	    // printf ( "%3d Enum %s %2d %08x", i, wstr, ep->count, ep->addr );
	    // printf ( "%3d Enum %s %s %04x %2d %08x", i, wstr, sstr, ep->epr, ep->count, ep->addr );
	    printf ( "%3d Enum %s %s %04x %04x %2d", i, wstr, sstr, ep->istr, ep->epr, ep->count );
	    print_buf ( ep->data, ep->count );
	}

	printf ( "Total bytes saved: %d\n", s_count );
}

void
enum_log_init ( void )
{
	e_count = 0;
	s_count = 0;
	memset ( save_buf, 0xaa, SAVE_SIZE );
}

/* This will wait forever until enumeration finishes and
 * we get configured, then it will report on the enumeration log
 */
void
enum_log_watch ( void )
{
	enum_log_init ();

	while ( ! is_papoon_configured () )
	    ;

	enum_log_show ();
}

static char *
enum_saver ( u32 addr, int count )
{
	char *rv;
	count &= 0x3ff;

	if ( count == 0 )
	    return (char *) 0;

	rv = &save_buf[s_count];
	pma_copy_in ( addr, rv, count );

	s_count += count;
	return rv;
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
