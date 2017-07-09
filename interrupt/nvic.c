/* Nvic.c
 * (c) Tom Trebisky  7-7-2017
 *
 * Driver for the STM32F103 NVIC interrupt controller
 */

struct nvic {
	volatile unsigned long iser[3];	/* 00 */
	volatile unsigned long icer[3];	/* 0c */
	/* ... */
};

#define NVIC_BASE   ((struct nvic *) 0xe000e100)

#define NUM_IRQ	68

void
nvic_enable ( int irq )
{
	struct nvic *np = NVIC_BASE;

	if ( irq >= NUM_IRQ )
	    return;

	np->iser[irq/32] = 1 << (irq%32);
}

/* THE END */
