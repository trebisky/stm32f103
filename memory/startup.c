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

void
init_vars ( void )
{
	unsigned int *src = &__text_end;
	unsigned int *p;

}

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

/* THE END */
