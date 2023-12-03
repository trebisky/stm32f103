/* startup.c
 * (c) Tom Trebisky  9-14-2020
 *
 * This runs before main.
 */

extern unsigned int __text_end;

extern unsigned int __data_start;
extern unsigned int __data_end;

extern unsigned int __bss_start;
extern unsigned int __bss_end;

/* Now startup() is in main.c and calls this.
 * I do this so I can initialize the uart early
 * and then do printf from in here.
 * Using printf() in here is no longer possible !!
 */
void
mem_init ( void )
{
	unsigned int *src = &__text_end;
	unsigned int *p;
	int count;

	// printf ( "Bss: %08x\n", &__bss_start );
	// printf ( "Bss: %08x\n", &__bss_end );
	// printf ( "P  : %08x\n", &p );

	/* Zero BSS */
	for ( p = &__bss_start; p < &__bss_end; p++ )
	    *p = 0;

	count = &__bss_end - &__bss_start;
	count--;

	// printf ( "%d bytes of BSS cleared\n", count );

	// init_vars ();

	/* Copy initialized data from flash */
	for ( p = &__data_start; p < &__data_end; p++ )
	    *p = *src++;

	count = &__data_end - &__data_start;
	count--;

	// printf ( "%d bytes of Data initialized\n", count );
}

#ifdef notdef
/* This gets called from locore.s */
void
startup ( void )
{
	unsigned int *src = &__text_end;
	unsigned int *p;

	/* Zero BSS */
	for ( p = &__bss_start; p < &__bss_end; p++ )
	    *p = 0;

	// init_vars ();

	/* Copy initialized data from flash */
	for ( p = &__data_start; p < &__data_end; p++ )
	    *p = *src++;

	main ();
}
#endif

/* THE END */
