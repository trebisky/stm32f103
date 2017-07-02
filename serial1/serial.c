/* serial.c
 * (c) Tom Trebisky  7-2-2017
 *
 * Driver for the STM32F103 usart
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

/* One of the 3 usarts */
struct usart {
	volatile unsigned long status;
	volatile unsigned long data;
	volatile unsigned long baud;
	volatile unsigned long cr1;
	volatile unsigned long cr2;
	volatile unsigned long cr3;
	volatile unsigned long gtp;	/* guard time and prescaler */
};

#define USART1_BASE	(struct usart *) 0x40013800
#define USART2_BASE	(struct usart *) 0x40004400
#define USART3_BASE	(struct usart *) 0x40004800

/* THE END */
