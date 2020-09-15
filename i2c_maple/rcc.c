/* rcc.c
 * (c) Tom Trebisky  7-2-2017
 */

#define BIT(nr)		(1<<(nr))

/* The reset and clock control module */
struct rcc {
	volatile unsigned long ccr;	/* 0 - clock control */
	volatile unsigned long cfg;	/* 4 - clock config */
	volatile unsigned long cir;	/* 8 - clock interrupt */
	volatile unsigned long apb2_rr;	/* 0c - peripheral reset */
	volatile unsigned long apb1_rr;	/* 10 - peripheral reset */
	volatile unsigned long ahb_enr;	/* 14 - peripheral enable */
	volatile unsigned long apb2_enr;	/* 18 - peripheral enable */
	volatile unsigned long apb1_enr;	/* 1c - peripheral enable */
	volatile unsigned long bdcr;	/* 20 - xx */
	volatile unsigned long csr;	/* 24 - xx */
};

#define RCC_BASE	(struct rcc *) 0x40021000

#define FLASH_ACR	((volatile unsigned long *) 0x40022000)

#define FLASH_PREFETCH	0x0010	/* enable prefetch buffer */
#define FLASH_HCYCLE	0x0008	/* enable half cycle access */

#define FLASH_WAIT0	0x0000	/* for sysclk <= 24 Mhz */
#define FLASH_WAIT1	0x0001	/* for 24 < sysclk <= 48 Mhz */
#define FLASH_WAIT2	0x0002	/* for 48 < sysclk <= 72 Mhz */


/* These are in the apb2_enr register */
#define GPIOA_ENABLE	0x04
#define GPIOB_ENABLE	0x08
#define GPIOC_ENABLE	0x10
#define AFIO_ENABLE	0x01

#define TIMER1_ENABLE	0x0800
#define UART1_ENABLE	0x4000

/* These are in the apb1_enr register */
#define TIMER2_ENABLE	0x0001
#define TIMER3_ENABLE	0x0002
#define TIMER4_ENABLE	0x0004

#define UART2_ENABLE	0x20000
#define UART3_ENABLE	0x40000

// #define I2C1_ENABLE	BIT(21)
#define I2C1_ENABLE	0x00200000	/* bit 21 */
#define I2C2_ENABLE	0x00400000	/* bit 22 */
#define USB_ENABLE	0x00800000	/* bit 23 */

/* The apb2_rr and apb1_rr registers hold reset control bits */

/* Bits in the clock control register CCR */
#define PLL_ENABLE	0x01000000
#define PLL_LOCK	0x02000000	/* status */

#define HSI_ON		1
#define HSE_ON		0x10000
#define HSE_TRIM	0x80

#define CCR_NORM	(HSI_ON | HSE_ON | HSE_TRIM)

/* Bits in the cfg register */
#define	SYS_HSI		0x00	/* reset value - 8 Mhz internal RC */
#define	SYS_HSE		0x01	/* external high speed clock */
#define	SYS_PLL		0x02	/* HSE multiplied by PLL */

#define AHB_DIV2	0x80

#define APB1_DIV2	(4<<8)	/* 36 Mhz max */
#define APB1_DIV4	(5<<8)	/* 36 Mhz max */

#define APB2_DIV2	(4<<11)	/* 72 Mhz max */
#define APB2_DIV4	(5<<11)	/* 72 Mhz max */

/* Note that the HSI clock is always divided by 2 pre PLL */

#define PLL_HSI		0x00000
#define PLL_HSE		0x10000	/* 1 is HSE, 0 is HSI/2 */
#define PLL_XTPRE	0x20000	/* divide HSE by 2 pre PLL */
#define PLL_HSE2	0x30000	/* HSE/2 feeds PLL */

#define PLL_2		0
#define PLL_4		(2<<18)
#define PLL_5		(3<<18)
#define PLL_6		(4<<18)
#define PLL_7		(5<<18)
#define PLL_8		(6<<18)
#define PLL_9		(7<<18)	/* multiply by 9 to get 72 Mhz */

/* It works to run at 80, but we may warp the universe ... */
#define PLL_10		(8<<18)	/* XXX - danger !! */

/* These must be maintained by hand */
#define PCLK1		36000000
#define PCLK2		72000000

/* To use USB, we must run the clock at 48 or 72 since we can only
 * divide by 1.0 or 1.5 to get the USB clock, which must be 48
 */

int
get_pclk1 ( void )
{
	return PCLK1;
}

int
get_pclk2 ( void )
{
	return PCLK2;
}

/* The processor comes out of reset using HSI (an internal 8 Mhz RC clock) */
static void
rcc_clocks ( void )
{
	struct rcc *rp = RCC_BASE;

	// rp->cfg = PLL_HSI | PLL_2 | SYS_HSI;
	// rp->cfg = PLL_HSI | PLL_8 | SYS_HSI;
	// rp->cfg = PLL_HSE2 | PLL_8 | SYS_HSI;
	// rp->cfg = PLL_HSE | PLL_4 | SYS_HSI;
	// rp->cfg = PLL_HSE | PLL_4 | SYS_HSI | APB1_DIV2;	/* OK */
	// rp->cfg = PLL_HSE | PLL_6 | SYS_HSI | APB1_DIV2;
	rp->cfg = PLL_HSE | PLL_9 | SYS_HSI | APB1_DIV2;
	// rp->cfg = PLL_HSE | PLL_10 | SYS_HSI | APB1_DIV2;

	/* How you set this is tricky
	 * Using |= fails.  Consider the bit band access.
	 * Setting the entire register works.
	 */
	rp->ccr = CCR_NORM | PLL_ENABLE;

	while ( ! (rp->ccr & PLL_LOCK ) )
	   ;

	/* Need flash wait states when we boost the clock */
	* FLASH_ACR = FLASH_PREFETCH | FLASH_WAIT2;

	// rp->cfg = SYS_HSI;	/* OK - 8 Mhz */
	// rp->cfg = SYS_HSE;	/* OK - 8 Mhz */
	// rp->cfg = PLL_HSI | PLL_2 | SYS_PLL;	/* OK */
	// rp->cfg = PLL_HSI | PLL_8 | SYS_PLL;	/* OK */
	// rp->cfg = PLL_HSE2 | PLL_8 | SYS_PLL;	/* OK */
	// rp->cfg = PLL_HSE | PLL_4 | SYS_PLL;
	// rp->cfg = PLL_HSE | PLL_4 | SYS_PLL | APB1_DIV2;	/* OK */
	// rp->cfg = PLL_HSE | PLL_6 | SYS_PLL | APB1_DIV2;
	rp->cfg = PLL_HSE | PLL_9 | SYS_PLL | APB1_DIV2;
	// rp->cfg = PLL_HSE | PLL_10 | SYS_PLL | APB1_DIV2;
}

void
rcc_init ( void )
{
	struct rcc *rp = RCC_BASE;

	rcc_clocks ();

	/* Turn on all the GPIO */
	rp->apb2_enr |= GPIOA_ENABLE;
	rp->apb2_enr |= GPIOB_ENABLE;
	rp->apb2_enr |= GPIOC_ENABLE;
	rp->apb2_enr |= AFIO_ENABLE;

	rp->apb2_enr |= UART1_ENABLE;

	rp->apb1_enr |= UART2_ENABLE;
	rp->apb1_enr |= UART3_ENABLE;

	rp->apb1_enr |= TIMER2_ENABLE;
	rp->apb1_enr |= I2C1_ENABLE;
	rp->apb1_enr |= I2C2_ENABLE;

	rp->apb1_enr = 0xffffffff;
	rp->apb2_enr = 0xffffffff;
}

#define I2C1_RESET	0x00200000	/* bit 21 */
#define I2C2_RESET	0x00400000	/* bit 22 */

void
i2c_reset ( void )
{
	struct rcc *rp = RCC_BASE;

	rp->apb1_rr = (I2C1_RESET | I2C2_RESET);
	rp->apb1_rr = (I2C1_RESET | I2C2_RESET);
	rp->apb1_rr = (I2C1_RESET | I2C2_RESET);
	rp->apb1_rr = (I2C1_RESET | I2C2_RESET);
	rp->apb1_rr = 0;
}

void
rcc_show ( void )
{
	struct rcc *rp = RCC_BASE;

	show_reg ( "apb1 reset:", &rp->apb1_rr );
	show_reg ( "apb2 reset:", &rp->apb2_rr );
	show_reg ( "apb1 ena:", &rp->apb1_enr );
	show_reg ( "apb2 ena:", &rp->apb2_enr );
	show_reg ( "ahb  ena:", &rp->ahb_enr );
}

/* THE END */
