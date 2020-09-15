/* startup.c
 * (c) Tom Trebisky  9-14-2020
 *
 * This runs before main.
 */

extern int __bss_start;
extern int __bss_end;


/* This gets called from locore.s */
void
startup ( void )
{
	int *p;

	for ( p = &__bss_start; p < &__bss_end; p++ )
	    *p = 0;
	main ();
}

/* THE END */
