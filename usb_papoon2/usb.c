/* usb.c
 *
 * (c) Tom Trebisky  11-5-2023
 *
 * A small bit of C code that mostly is
 * a relay to code in papoon.
 * look for baboon.cxx in the papoon directory.
 *
 *  IRQ 19 -- USB high priority (or CAN Tx)
 *  IRQ 20 -- USB low priority (or CAN Rx0)
 *  IRQ 42 -- USB wakeup from suspend via EXTI line
 */

#define USB_HP_IRQ	19
#define USB_LP_IRQ	20
#define USB_WK_IRQ	42

/* Interrupt handlers,
 * called by vectors in locore.s
 */
void
usb_hp_handler ( void )
{
	printf ( "USB hp interrupt\n" );
	papoon_handler ();
}

void
usb_lp_handler ( void )
{
	printf ( "USB lp interrupt\n" );
	papoon_handler ();
}

void
usb_wk_handler ( void )
{
	printf ( "USB wk interrupt\n" );
	papoon_handler ();
}

/* On the first try I get endless "lp" interrupts.
 */
void
usb_init ( void )
{
	/* Probably only need HP */
	nvic_enable ( USB_HP_IRQ );
	nvic_enable ( USB_LP_IRQ );
	nvic_enable ( USB_WK_IRQ );

	papoon_init ();

	papoon_demo ();
}

/* THE END */
