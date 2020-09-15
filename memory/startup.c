/* startup.c
 * (c) Tom Trebisky  9-14-2020
 *
 * This runs before main.
 */

extern int __text_end;

extern int __data_start;
extern int __data_end;

extern int __bss_start;
extern int __bss_end;

void
init_vars ( void )
{
	unsigned int *src = &__text_end;
	unsigned int *p;

	for ( p = &__data_start; p < (unsigned int *) &__data_end; p++ )
	    *p = *src++;
}

/* This gets called from locore.s */
void
startup ( void )
{
	int *p;

	for ( p = &__bss_start; p < &__bss_end; p++ )
	    *p = 0;

	// init_vars ();

	main ();
}

/* THE END */
