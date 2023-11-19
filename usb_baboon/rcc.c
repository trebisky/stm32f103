/* rcc.c
 * (c) Tom Trebisky  7-2-2017
 */

typedef volatile unsigned int vu32;
#define BIT(x)	(1<<x)

/* The reset and clock control module */
struct rcc {
	vu32 cr;	/* 0 - clock control */
	vu32 cfg;	/* 4 - clock config */
	vu32 cir;	/* 8 - clock interrupt */
	vu32 apb2r;	/* 0c - peripheral reset */
	vu32 apb1r;	/* 10 - peripheral reset */
	vu32 ahbe;	/* 14 - peripheral enable */
	vu32 apb2e;	/* 18 - peripheral enable */
	vu32 apb1e;	/* 1c - peripheral enable */
	vu32 bdcr;	/* 20 - xx */
	vu32 csr;	/* 24 - xx */
};

#define RCC_BASE	(struct rcc *) 0x40021000

#define FLASH_ACR	((volatile unsigned long *) 0x40022000)

#define FLASH_PREFETCH	0x0010	/* enable prefetch buffer */
#define FLASH_HCYCLE	0x0008	/* enable half cycle access */

#define FLASH_WAIT0	0x0000	/* for sysclk <= 24 Mhz */
#define FLASH_WAIT1	0x0001	/* for 24 < sysclk <= 48 Mhz */
#define FLASH_WAIT2	0x0002	/* for 48 < sysclk <= 72 Mhz */

/* These are in the ahbe register */
#define DMA1_ENABLE	0x01
#define DMA2_ENABLE	0x02

/* These are in the apb2e register */
#define AFIO_ENABLE	0x01
#define GPIOA_ENABLE	0x04
#define GPIOB_ENABLE	0x08
#define GPIOC_ENABLE	0x10
#define TIMER1_ENABLE	0x0800
#define UART1_ENABLE	0x4000

/* These are in the apb1e register */
#define TIMER2_ENABLE	0x0001
#define TIMER3_ENABLE	0x0002
#define TIMER4_ENABLE	0x0004
#define UART2_ENABLE	0x20000
#define UART3_ENABLE	0x40000

#define USB_ENABLE	0x800000
#define USB_RESET	0x800000

/* The apb2r and apb1r registers hold reset control bits */

/* Bits in the clock control register CR */
#define PLL_ENABLE	0x01000000
#define PLL_LOCK	0x02000000	/* status */

#define HSI_ON		1
#define HSE_ON		0x10000		// bit16
#define HSE_TRIM	0x80
#define HSE_RDY		BIT(17)

#define CR_NORM	(HSI_ON | HSE_ON | HSE_TRIM)

/* Bits in the cfg register */
#define	SYS_HSI		0x00	/* reset value - 8 Mhz internal RC */
#define	SYS_HSE		0x01	/* external high speed clock */
#define	SYS_PLL		0x02	/* HSE multiplied by PLL */

#define USB_PRE		BIT(22)

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
 * Set this bit in the CFG register to divide by 1.0 (else get 1.5).
 * 72/1.5 = 48
 */
#define USB_NODIV	0x400000

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

/* The processor comes out of reset using HSI (an internal 8 Mhz RC clock)
 * This sets a 9x multiplier for the PLL to give us 72 Mhz.
 * Note that we do NOT set the USB_NODIV bit, so this gets divided
 * by 1.5 to give us the 48 Mhz USB clock we need.
 *
 * HSE is a high speed external clock, no doubt the 8 Mhz crystal on a
 * blue pill or perhaps a crystal oscillator on a Maple board.
 */
static void
rcc_clocks ( void )
{
	struct rcc *rp = RCC_BASE;

#ifdef notdef
	rp->cr = HSE_ON;
	while ( ! (rp->cr & HSE_RDY) )
	    ;
#endif

	rp->cfg = PLL_HSE | PLL_9 | SYS_HSI | APB1_DIV2;

	/* How you set this is tricky
	 * Using |= fails.  Consider the bit band access.
	 * Setting the entire register works.
	 */
	rp->cr = CR_NORM | PLL_ENABLE;

	while ( ! (rp->cr & PLL_LOCK ) )
	   ;

	/* Need flash wait states when we boost the clock */
	* FLASH_ACR = FLASH_PREFETCH | FLASH_WAIT2;

	/* USB_PRE is not set, which is correct for a 72 Mhz
	 * system clock.  0 says divide by 3 (144/3 = 48)
	 * 1 says dvide by 2 (96/2 = 48)
	 * (The PLL clock runs double the system clock).
	 */
	rp->cfg = PLL_HSE | PLL_9 | SYS_PLL | APB1_DIV2;
}

void
rcc_init ( void )
{
	struct rcc *rp = RCC_BASE;

	/* set up the main (PLL) clock */
	rcc_clocks ();

	/* Turn on all the GPIO */
	rp->apb2e |= GPIOA_ENABLE;
	rp->apb2e |= GPIOB_ENABLE;
	rp->apb2e |= GPIOC_ENABLE;

	rp->apb2e |= AFIO_ENABLE;

	/* Turn on USART 1 */
	rp->apb2e |= UART1_ENABLE;

	// rp->apb2e |= TIMER1_ENABLE;
	rp->apb1e |= TIMER2_ENABLE;

	rp->apb1e |= USB_ENABLE;

	// rp->apb1e |= UART2_ENABLE;
	// rp->apb1e |= UART3_ENABLE;

	rp->ahbe |= DMA1_ENABLE;

}

#ifdef notyet
void
rcc_usb_reset ( void )
{
	struct rcc *rp = RCC_BASE;

	rp->apb1r &= ~USB_RESET;
}
#endif

#ifdef notdef
/* From papoon */

void usb_mcu_init()
{
//    rcc->cr |= Rcc::Cr::HSEON;
//    while ( !rcc->cr.any(Rcc::Cr::HSERDY) )
//	;

    rcc->cfgr |=   Rcc::Cfgr::HPRE_DIV_1
                 | Rcc::Cfgr::PPRE2_DIV_1
                 | Rcc::Cfgr::PPRE1_DIV_1;

    rcc->cfgr.ins(  Rcc::Cfgr::PLLSRC     // PLL input from HSE (8 MHz)
                  | Rcc::Cfgr::PLLMULL_9);// Multiply by 9 (8*9=72 MHz)

    rcc->cr |= Rcc::Cr::PLLON;
    while ( !rcc->cr.any(Rcc::Cr::PLLRDY) )
	;

    rcc->cfgr.ins(Rcc::Cfgr::SW_PLL);           // use PLL as system clock
    while ( !rcc->cfgr.all(Rcc::Cfgr::SWS_PLL) )  // wait for confirmation
	;

    rcc->cfgr.clr(Rcc::Cfgr::USBPRE);           // 1.5x USB prescaler

//     rcc->apb1enr |= Rcc::Apb1enr::USBEN;        // enable USB peripheral

#ifdef USB_DEV_DMA_PMA
//     rcc->ahbenr |= Rcc::Ahbenr::DMA1EN;
#endif

//    // enable GPIOs and alternate functions
//    rcc->apb2enr |=   Rcc::Apb2enr::IOPAEN
//                    | Rcc::Apb2enr::IOPCEN
//                    | Rcc::Apb2enr::AFIOEN;
}  // usb_mcu_init()
#endif

/* THE END */
