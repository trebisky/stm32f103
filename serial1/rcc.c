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

#define RCC_BASE	(struct rcc *) 0x40021000

/* These are in the ape2 register */
#define GPIOA_ENABLE	0x04
#define GPIOB_ENABLE	0x08
#define GPIOC_ENABLE	0x10
#define UART1_ENABLE	0x4000

/* These are in the ape1 register */
#define UART2_ENABLE	0x20000
#define UART3_ENABLE	0x40000

/* The apb2 and apb1 registers hold reset control bits */

void
rcc_init ( void )
{
	struct rcc *rp = RCC_BASE;

	/* Turn on all the GPIO */
	rp->ape2 |= GPIOA_ENABLE;
	rp->ape2 |= GPIOB_ENABLE;
	rp->ape2 |= GPIOC_ENABLE;

	/* Turn on USART 1 */
	rp->ape2 |= UART1_ENABLE;

	// rp->ape1 |= UART2_ENABLE;
	// rp->ape1 |= UART3_ENABLE;
}

/* THE END */
