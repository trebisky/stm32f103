/* rcc.c
 * (c) Tom Trebisky  7-2-2017
 */

/* The reset and clock control module */
struct rcc {
	volatile unsigned long rc;	/* 0 - clock control */
	volatile unsigned long cfg;	/* 4 - clock config */
	volatile unsigned long cir;	/* 8 - clock interrupt */
	volatile unsigned long apb2;	/* c - peripheral reset */
	volatile unsigned long apb1;	/* 10 - peripheral reset */
	volatile unsigned long ape3;	/* 14 - peripheral enable */
	volatile unsigned long ape2;	/* 18 - peripheral enable */
	volatile unsigned long ape1;	/* 1c - peripheral enable */
	volatile unsigned long bdcr;	/* 20 - xx */
	volatile unsigned long csr;	/* 24 - xx */
};

#define GPIOA_ENABLE	0x04
#define GPIOB_ENABLE	0x08
#define GPIOC_ENABLE	0x10

#define UART1_ENABLE	0x4000
#define UART2_ENABLE	0x20000
#define UART3_ENABLE	0x40000

#define RCC_BASE	(struct rcc *) 0x40021000

/* As well as enabling peripherals, each peripheral may have
 * clocks to be enabled as well */

void
rcc_init ( void )
{
	struct rcc *rp = RCC_BASE;

	rp->ape2 |= GPIOA_ENABLE;
	rp->ape2 |= GPIOB_ENABLE;
	rp->ape2 |= GPIOC_ENABLE; /* Turn on GPIO C */

	/* Turn on USART 1 */
	rp->apb2 |= UART1_ENABLE;

	// rp->apb1 |= UART2_ENABLE;
	// rp->apb1 |= UART3_ENABLE;
}

/* THE END */
