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
        vu32 istatus; /* 44 - interrupt status */
        vu32 fnr;     /* 48 - frame number */
        vu32 daddr;   /* 4c - device address */
        vu32 btable;  /* 50 - buffer table */
};

/* Bits in the Control Register */
#define CTRL_CTRM	BIT(15)
#define CTRL_RESETM	BIT(10)
#define CTRL_FRES	BIT(0)

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

void
usb_lp_handler ( void )
{
        struct usb *up = USB_BASE;

	if ( debug )
	    printf ( "USB lp interrupt %d\n", ++lp_count );

#ifdef notdef
	/* It turns out the "force reset" bit thing is more or less useless
	 * and not what we want to do.  More on this later.
	 */
	if ( up->istatus & CTRL_RESETM ) {
	    printf ( "USB istatus before reset: %04x\n", up->istatus );
	    force_reset ();
	    /* The datasheet recommends clearning in this fashion.
	     * The datasheet labels the interrupt bits as rc_w0
	     * This means the bit can be read,
	     * Writing 0 clears it, Writing 1 does nothing.
	     */
	    up->istatus = CTRL_CTRM;
	    printf ( "USB istatus after reset: %04x\n", up->istatus );
	}
#endif

	papoon_handler ();
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
	int i;
	u32 *p;

        p = USB_RAM;
	for ( i=0; i<256; i++ ) 
	    *p++ = 0;

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
        printf ( "ISTAT = %08x\n", up->istatus );
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
	    delay_ms ( 1 );
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

	// fairly useless
	papoon_wait ( 10 );

	// pma_show ();

	// debug = 1;
	// papoon_debug ();

	// test1 ();
	test2 ();
	// test3 ();
}

/* THE END */
