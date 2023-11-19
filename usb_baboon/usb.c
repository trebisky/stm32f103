/* usb.c
 *
 * (c) Tom Trebisky  11-5-2023
 *
 * Currently this hands off most of the work
 * to papoon.
 * look for baboon.cxx in the papoon directory.
 *
 * More and more is being done here by this C code.
 *
 *  IRQ 19 -- USB high priority (or CAN Tx)
 *  IRQ 20 -- USB low priority (or CAN Rx0)
 *  IRQ 42 -- USB wakeup from suspend via EXTI line
 */

typedef unsigned short u16;
typedef unsigned int u32;
typedef volatile unsigned int vu32;

#define BIT(x)	(1<<x)

static void force_reset ( void );

#define USB_BASE        (struct usb *) 0x40005C00
#define USB_RAM         (u32 *) 0x40006000

/* 8 endpoints are allowed */
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

/* Interrupt bits (shared by control and isr */
#define INT_CTR		BIT(15)
#define INT_PMA		BIT(14)
#define INT_ERR		BIT(13)
#define INT_WKUP	BIT(12)
#define INT_SUSP	BIT(11)
#define INT_RESET	BIT(10)
#define INT_SOF		BIT(9)
#define INT_ESOF	BIT(8)

/* Bits in the Control Register */
#define CTRL_RESUME	BIT(4)
#define CTRL_FSUSP	BIT(3)
#define CTRL_LP_MODE	BIT(2)
#define CTRL_PDWN	BIT(1)
#define CTRL_FRES	BIT(0)

/* Bits in endpoint registers */
#define EP_CTR_RX	BIT(15)
#define EP_DTOG_RX	BIT(14)

/* Hardware sets NAK along with CTR_RX */
/* These bits toggle when a 1 is written
 * Writing 0 does nothing
 */
#define EP_RX_DIS		0
#define EP_RX_STALL		1<<12
#define EP_RX_NAK		2<<12
#define EP_RX_VALID		3<<12

#define EP_SETUP	BIT(11)

#define EP_TYPE_BULK		0
#define EP_TYPE_CONTROL		1<<9
#define EP_TYPE_ISO		2<<9
#define EP_TYPE_INT		3<<9

/* Depends on Type (modifies type) */
#define EP_KIND		BIT(8)

#define EP_CTR_TX	BIT(7)
#define EP_DTOG_TX	BIT(6)

/* Hardware sets NAK along with CTR_RX */
/* These bits toggle when a 1 is written
 * Writing 0 does nothing
 */
#define EP_TX_DIS		0
#define EP_TX_STALL		1<<4
#define EP_TX_NAK		2<<4
#define EP_TX_VALID		3<<4

#define EP_ADDR		0xf	// 4 bits

/* =================================== */

#define USB_HP_IRQ	19
#define USB_LP_IRQ	20
#define USB_WK_IRQ	42

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
static int debug = 0;

static int lp_count = 0;

/* As soon as we plug the cable in, we start getting
 * lots of SOF interrupts, before and during and
 * after enumeration.  The host causes this.
 */
static int sof_count = 0;

void
usb_lp_handler ( void )
{
        struct usb *up = USB_BASE;

	if ( debug )
	    printf ( "USB lp interrupt %d\n", ++lp_count );

	/* We see 913 SOF per second.
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
	    up->isr = INT_CTR | INT_RESET;

	    /*
	    if ( sof_count > 5000 )
		printf ( "Last SOF, final isr, ctrl = %04x %04x\n", up->isr, up->ctrl );
	    */

	    sof_count++;
	}

#ifdef notdef
	/* It turns out the "force reset" bit thing is more or less useless
	 * and not what we want to do.  More on this later.
	 */
	if ( up->isr & CTRL_RESETM ) {
	    printf ( "USB isr before reset: %04x\n", up->isr );
	    force_reset ();
	    /* The datasheet recommends clearning in this fashion.
	     * The datasheet labels the interrupt bits as rc_w0
	     * This means the bit can be read,
	     * Writing 0 clears it, Writing 1 does nothing.
	     */
	    up->isr = CTRL_CTRM;
	    printf ( "USB isr after reset: %04x\n", up->isr );
	}
#endif

	papoon_handler ();
}

/* Quick and dirty */
static void
memset ( char *buf, int val, int count )
{
	char *p = buf;
	int i;

	for ( i=0; i<count; i++ )
	    *p++ = val;
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

void
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

void
pma_show ( void )
{
        int i, j;
        u32 *p;
        u16 *addr;
        struct usb *up = USB_BASE;

        printf ( "BTABLE = %08x\n", up->btable );
        printf ( "CTRL  = %08x\n", up->ctrl );
        printf ( "ISR   = %08x\n", up->isr );
        printf ( "EPR-0 = %08x\n", up->epr[0] );
        printf ( "EPR-1 = %08x\n", up->epr[1] );

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

/* 4 * 4 bytes = 16, we have 4 of these
 */
struct btable_entry {
	u32	tx_addr;
	u32	tx_count;
	u32	rx_addr;
	u32	rx_count;
};

/* Display stuff for a specific endpoint pair
 */
void
endpoint_show ( int ep )
{
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
	struct btable_entry *bte;
        struct usb *up = USB_BASE;
	char *buf;


	printf ( "EPR %d = %04x\n", ep, up->epr[ep] );

	if ( ep < 4 ) {
	    bte = &bt[ep];
	    printf ( "BTE ep-%d @ %08x\n", ep, bte );
	    buf = (char *) USB_RAM + 2 * bte->tx_addr;
	    printf ( "BTE, ep-%d tx addr  = %04x - %08x\n", ep, bte->tx_addr, buf );
	    printf ( "BTE, ep-%d tx count = %04x\n", ep, bte->tx_count );

	    buf = (char *) USB_RAM + 2 * bte->rx_addr;
	    printf ( "BTE, ep-%d rx addr  = %04x - %08x\n", ep, bte->rx_addr, buf );
	    printf ( "BTE, ep-%d rx count = %04x\n", ep, bte->rx_count );
	}
}

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

#define CNTR_CTRM	BIT(15)
#define CNTR_RESETM	BIT(10)
#define CNTR_FRES	BIT(0)

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

/* I see 913 SOF per second.
 * My delay_ms may not be precise, so this is an estimate.
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

/* We see 287 bytes */
#define SAVE_SIZE	400

static char save_buf[SAVE_SIZE];
static int s_count = 0;

/* Copy from PMA memory to buffer */
static void
pma_copy ( u32 pma_off, char *buf, int count )
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

static char *
enum_saver ( u32 addr, int count )
{
	char *rv;
	count &= 0x3ff;

	if ( count == 0 )
	    return (char *) 0;

	rv = &save_buf[s_count];
	pma_copy ( addr, rv, count );

	s_count += count;
	return rv;
}

static void
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

/* ***************************************** */
/* Called from papoon on CTR interrupts on
 * endpoint 0
 * what = 0 is Rx, 1 is Tx
 */
void
tjt_enum_logger ( int what )
{
	struct enum_log_e *ep;
	struct btable_entry *bt = (struct btable_entry *) USB_RAM;
	struct btable_entry *bte;
	static int first = 0;

        struct usb *up = USB_BASE;
	u32 addr = (u32) USB_RAM;

	if ( first && what != 2 )
	    pma_show ();

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

/* This will wait forever until enumeration finishes and
 * we get configured, then it will report on the enumeration log
 */
static void
test5 ( void )
{
	s_count = 0;
	memset ( save_buf, 0xaa, SAVE_SIZE );

	while ( ! is_papoon_configured () )
	    ;

	enum_log_show ();
}

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
endpoint_init ( void )
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

/* After papoon_init() we have a chance to
 * fool around.  This may grow into our
 * own usb_init()
 */
void
hack_init ( void )
{
        struct usb *up = USB_BASE;
	u32 val;

	// up->ctrl |= INT_SOF;
}

/*
 *
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
usb_init ( void )
{
	/* Probably only need HP */
	nvic_enable ( USB_HP_IRQ );
	nvic_enable ( USB_LP_IRQ );
	nvic_enable ( USB_WK_IRQ );

	pma_clear ();

	// debug = 1;
	// papoon_debug ();

	papoon_init ();
	hack_init ();

	//usb_yank ();

	// fairly useless
	// papoon_wait ( 10 );

	// btable_show ();
	// pma_show ();

	// debug = 1;
	// papoon_debug ();

	// run echo demo
	// test1 ();

	// test2 ();
	// test3 ();
	// test4 ();

	// papoon_debug ();

	// run enumeration detailer
	test5 ();

	//delay_sec ( 4 );
	//printf ( "YANK\n" );
	//usb_yank ();

	// follow it with the papooon echo demo
	test1 ();
}

/* THE END */
