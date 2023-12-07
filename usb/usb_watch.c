/* usb_watch.c
 *
 * (c) Tom Trebisky  12-1-2023
 *
 * This is my USB enumeration watching tool.
 * I include a bunch of things that any sensible
 * and good programmer would keep in include files
 * in order to make this self contained.
 *
 * Public interface:
 *	void enum_log_init ( void );
 *	void enum_handler ( void );
 *	void enum_log_show ( void );
 *
 * The trick is knowing when to call "show"
 *
 * If you didn't want to call "handler" you
 *    could make your own calls to:
 * void enum_logger ( int what );
 *
 * The what field:
 *	0 = Rx CTR
 *	1 = Tx CTR
 *	2 = Reset
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef volatile unsigned int vu32;

#define BIT(x)	(1<<x)

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

#define INT_CTR		BIT(15)
#define INT_RESET	BIT(10)

/* Bits in endpoint registers */
#define EP_CTR_RX	BIT(15)
#define EP_SETUP	BIT(11)
#define EP_CTR_TX	BIT(7)

/* 4 * 4 bytes = 16 bytes in ARM addr space.
 */
struct btable_entry {
	u32	tx_addr;
	u32	tx_count;
	u32	rx_addr;
	u32	rx_count;
};

/* ***************************************** */
/* ***************************************** */
/* Enumeration logging facility */
/* ***************************************** */

/* Our own private copy of these functions
 */
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

/* ***************************************** */
/* ***************************************** */

void enum_logger ( int );

/* The idea here is to capture enumeration activity
 * for later analysis without disturbing anything.
 * (i.e. a passive watcher)
 *
 * CALL THIS from the USB interrupt routine
 * as early on as possible.
 */
void
enum_handler ( void )
{
        struct usb *up = USB_BASE;
	int ep;

	if ( up->isr & INT_RESET )
	    enum_logger ( 2 );

	if ( up->isr & INT_CTR ) {
	    ep = up->isr & 0xf;
	    if ( ep == 0 ) {
		if ( up->epr[0] & EP_CTR_RX )
		    enum_logger ( 0 );
		if ( up->epr[0] & EP_CTR_TX )
		    enum_logger ( 1 );
	    }
	}
}

static char * enum_saver ( u32, int );

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

#define ENUM_LIMIT	50

/* We see 36 entries */
static struct enum_log_e e_log[ENUM_LIMIT];
static int e_count = 0;

/* We see 363 bytes */
#define SAVE_SIZE	400

static char save_buf[SAVE_SIZE];
static int s_count = 0;

void
enum_logger ( int what )
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

	if ( e_count >= ENUM_LIMIT )
	    return;

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

#ifdef notdef
	if ( first && what != 2 ) {
	    printf ( "buf (%d): ", ep->count );
	    print_buf ( ep->data, ep->count );
	    printf ( "\n" );
	    first = 0;
	}
#endif

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

#ifdef notdef
/* This will wait forever until enumeration finishes and
 * we get configured, then it will report on the enumeration log
 */
void
enum_log_watch ( void )
{
	int ticks = 5;

	enum_log_init ();

	if ( use_P ) {
	    while ( ! is_papoon_configured () )
		;
	    enum_log_show ();
	    use_P = 0;

	    return;
	}

	while ( ticks-- )
	    delay_ms ( 1000 );

	enum_log_show ();
}
#endif

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

/* THE END */
