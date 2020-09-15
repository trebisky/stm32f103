/* glue.c
 * (c) Tom Trebisky  9-11-2020
 *
 * These are routines that would ordinarily be supplied
 * by libmaple and that are required by maple_i2c.c
 */

#include <libmaple/gpio.h>
#include <libmaple/rcc.h>
#include <libmaple/nvic.h>
#include <libmaple/scb.h>
#include <libmaple/libmaple.h>
#include <libmaple/bitband.h>

/* --- from stm32f1/gpio.c --- */

gpio_dev gpiob = {
    .regs      = GPIOB_BASE,
    .clk_id    = RCC_GPIOB,
    .exti_port = EXTI_PB,
};

/** GPIO port B device. */
// gpio_dev* const GPIOB = &gpiob;

void
gpio_set_mode ( gpio_dev *dev, uint8 pin, gpio_pin_mode mode)
{
    gpio_reg_map *regs = dev->regs;
    __io uint32 *cr = &regs->CRL + (pin >> 3);
    uint32 shift = (pin & 0x7) * 4;
    uint32 tmp = *cr;

    tmp &= ~(0xF << shift);
    tmp |= (mode == GPIO_INPUT_PU ? GPIO_INPUT_PD : mode) << shift;
    *cr = tmp;

    if (mode == GPIO_INPUT_PD) {
        regs->ODR &= ~(1U << pin);
    } else if (mode == GPIO_INPUT_PU) {
        regs->ODR |= (1U << pin);
    }
}

void
afio_remap ( afio_remap_peripheral remapping )
{
    if (remapping & AFIO_REMAP_USE_MAPR2) {
        remapping &= ~AFIO_REMAP_USE_MAPR2;
        AFIO_BASE->MAPR2 |= remapping;
    } else {
        AFIO_BASE->MAPR |= remapping;
    }
}

/* --- from systick.c --- */

volatile uint32 systick_uptime_millis;

void
systick_handler ( void )
{
	systick_uptime_millis++;
}
/* --- from nvic.c --- */

void
nvic_irq_set_priority ( nvic_irq_num irqn, uint8 priority )
{
    if (irqn < 0) {
        /* This interrupt is in the system handler block */
        SCB_BASE->SHP[((uint32)irqn & 0xF) - 4] = (priority & 0xF) << 4;
    } else {
        NVIC_BASE->IP[irqn] = (priority & 0xF) << 4;
    }
}

/* --- from stm32f1/rcc.c and rcc.c --- */

#define APB1                            RCC_APB1
#define APB2                            RCC_APB2
#define AHB                             RCC_AHB

struct rcc_dev_info {
    const rcc_clk_domain clk_domain;
    const uint8 line_num;
};

/* Device descriptor table, maps rcc_clk_id onto bus and enable/reset
 * register bit numbers. */
const struct rcc_dev_info rcc_dev_table[] = {
    [RCC_GPIOA]  = { .clk_domain = APB2, .line_num = 2 },
    [RCC_GPIOB]  = { .clk_domain = APB2, .line_num = 3 },
    [RCC_GPIOC]  = { .clk_domain = APB2, .line_num = 4 },
    [RCC_GPIOD]  = { .clk_domain = APB2, .line_num = 5 },
    [RCC_AFIO]   = { .clk_domain = APB2, .line_num = 0 },
    [RCC_ADC1]   = { .clk_domain = APB2, .line_num = 9 },
    [RCC_ADC2]   = { .clk_domain = APB2, .line_num = 10 },
    [RCC_ADC3]   = { .clk_domain = APB2, .line_num = 15 },
    [RCC_USART1] = { .clk_domain = APB2, .line_num = 14 },
    [RCC_USART2] = { .clk_domain = APB1, .line_num = 17 },
    [RCC_USART3] = { .clk_domain = APB1, .line_num = 18 },
    [RCC_TIMER1] = { .clk_domain = APB2, .line_num = 11 },
    [RCC_TIMER2] = { .clk_domain = APB1, .line_num = 0 },
    [RCC_TIMER3] = { .clk_domain = APB1, .line_num = 1 },
    [RCC_TIMER4] = { .clk_domain = APB1, .line_num = 2 },
    [RCC_SPI1]   = { .clk_domain = APB2, .line_num = 12 },
    [RCC_SPI2]   = { .clk_domain = APB1, .line_num = 14 },
    [RCC_DMA1]   = { .clk_domain = AHB,  .line_num = 0 },
    [RCC_PWR]    = { .clk_domain = APB1, .line_num = 28},
    [RCC_BKP]    = { .clk_domain = APB1, .line_num = 27},
    [RCC_I2C1]   = { .clk_domain = APB1, .line_num = 21 },
    [RCC_I2C2]   = { .clk_domain = APB1, .line_num = 22 },
    [RCC_CRC]    = { .clk_domain = AHB,  .line_num = 6},
    [RCC_FLITF]  = { .clk_domain = AHB,  .line_num = 4},
    [RCC_SRAM]   = { .clk_domain = AHB,  .line_num = 2},
    [RCC_USB]    = { .clk_domain = APB1, .line_num = 23},
#if defined(STM32_HIGH_DENSITY) || defined(STM32_XL_DENSITY)
    [RCC_GPIOE]  = { .clk_domain = APB2, .line_num = 6 },
    [RCC_GPIOF]  = { .clk_domain = APB2, .line_num = 7 },
    [RCC_GPIOG]  = { .clk_domain = APB2, .line_num = 8 },
    [RCC_UART4]  = { .clk_domain = APB1, .line_num = 19 },
    [RCC_UART5]  = { .clk_domain = APB1, .line_num = 20 },
    [RCC_TIMER5] = { .clk_domain = APB1, .line_num = 3 },
    [RCC_TIMER6] = { .clk_domain = APB1, .line_num = 4 },
    [RCC_TIMER7] = { .clk_domain = APB1, .line_num = 5 },
    [RCC_TIMER8] = { .clk_domain = APB2, .line_num = 13 },
    [RCC_FSMC]   = { .clk_domain = AHB,  .line_num = 8 },
    [RCC_DAC]    = { .clk_domain = APB1, .line_num = 29 },
    [RCC_DMA2]   = { .clk_domain = AHB,  .line_num = 1 },
    [RCC_SDIO]   = { .clk_domain = AHB,  .line_num = 10 },
    [RCC_SPI3]   = { .clk_domain = APB1, .line_num = 15 },
#endif
#ifdef STM32_XL_DENSITY
    [RCC_TIMER9]  = { .clk_domain = APB2, .line_num = 19 },
    [RCC_TIMER10] = { .clk_domain = APB2, .line_num = 20 },
    [RCC_TIMER11] = { .clk_domain = APB2, .line_num = 21 },
    [RCC_TIMER12] = { .clk_domain = APB1, .line_num = 6 },
    [RCC_TIMER13] = { .clk_domain = APB1, .line_num = 7 },
    [RCC_TIMER14] = { .clk_domain = APB1, .line_num = 8 },
#endif
};

rcc_clk_domain rcc_dev_clk(rcc_clk_id id) {
    return rcc_dev_table[id].clk_domain;
}

static inline void
rcc_do_clk_enable ( __io uint32** enable_regs, rcc_clk_id id)
{
    __io uint32 *enable_reg = enable_regs[rcc_dev_clk(id)];
    uint8 line_num = rcc_dev_table[id].line_num;

    bb_peri_set_bit(enable_reg, line_num, 1);
}

static inline void
rcc_do_reset_dev ( __io uint32** reset_regs, rcc_clk_id id)
{
    __io uint32 *reset_reg = reset_regs[rcc_dev_clk(id)];
    uint8 line_num = rcc_dev_table[id].line_num;

    bb_peri_set_bit(reset_reg, line_num, 1);
    bb_peri_set_bit(reset_reg, line_num, 0);
}

void rcc_clk_enable(rcc_clk_id id) {
    static __io uint32* enable_regs[] = {
        [APB1] = &RCC_BASE->APB1ENR,
        [APB2] = &RCC_BASE->APB2ENR,
        [AHB] = &RCC_BASE->AHBENR,
    };
    rcc_do_clk_enable(enable_regs, id);
}

void rcc_reset_dev(rcc_clk_id id) {
    static __io uint32* reset_regs[] = {
        [APB1] = &RCC_BASE->APB1RSTR,
        [APB2] = &RCC_BASE->APB2RSTR,
    };
    rcc_do_reset_dev(reset_regs, id);
}

/* Called when an ASSERT fails */
void
_fail ( const char*m1, int x, const char*m2 )
{
    serial_puts ( "FAIL\n");
    for ( ;; ) ;
}


/* THE END */
