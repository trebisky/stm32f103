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
#define TIMER1_ENABLE	0x0800
#define UART1_ENABLE	0x4000

/* These are in the ape1 register */
#define TIMER2_ENABLE	0x0001
#define TIMER3_ENABLE	0x0002
#define TIMER4_ENABLE	0x0004
#define UART2_ENABLE	0x20000
#define UART3_ENABLE	0x40000

/* The apb2 and apb1 registers hold reset control bits */

/* Bits in the clock control register */
#define PLL_ENABLE	0x01000000
#define PLL_LOCK	0x02000000	/* status */

/* Bits in the cfg register */
#define	SYS_HSI		0x00	/* reset value - 8 Mhz internal RC */
#define	SYS_HSE		0x01	/* external high speed clock */
#define	SYS_PLL		0x02	/* HSE multiplied by PLL */

#define AHB_DIV2	0x80
#define APB1_DIV2	(4<<8)	/* 36 Mhz max */
#define APB1_DIV4	(5<<8)	/* 36 Mhz max */
#define APB2_DIV2	(4<<11)	/* 72 Mhz max */
#define APB2_DIV4	(5<<11)	/* 72 Mhz max */

#define PLL_2		0
#define PLL_9		(7<<18)	/* multiply by 9 to get 72 Mhz */

/* The processor comes out of reset using an internal 8 Mhz RC clock */
static void
rcc_clocks ( void )
{
	struct rcc *rp = RCC_BASE;

	rp->cfg = PLL_2 | APB1_DIV4;
	// rp->cfg = PLL_9 | APB1_DIV4;
	rp->rc |= PLL_ENABLE;

	while ( ! (rp->rc & PLL_LOCK ) )
	   ;

	/* yields the unscaled 8 Mhz crystal */
	// rp->cfg = SYS_HSE;
	// rp->cfg = SYS_HSE | AHB_DIV2;

	rp->cfg = SYS_PLL;
}

void
rcc_init ( void )
{
	struct rcc *rp = RCC_BASE;

	rcc_clocks ();

	/* Turn on all the GPIO */
	rp->ape2 |= GPIOA_ENABLE;
	rp->ape2 |= GPIOB_ENABLE;
	rp->ape2 |= GPIOC_ENABLE;

	/* Turn on USART 1 */
	rp->ape2 |= UART1_ENABLE;

	rp->ape1 |= TIMER2_ENABLE;

	// rp->ape1 |= UART2_ENABLE;
	// rp->ape1 |= UART3_ENABLE;

}

/* THE END */
