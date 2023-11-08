// papoon_usb: "Not Insane" USB library for STM32F103xx MCUs
// Copyright (C) 2019,2020 Mark R. Rubin
//
// This file is part of papoon_usb.
//
// The papoon_usb program is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// The papoon_usb program is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// (LICENSE.txt) along with the papoon_usb program.  If not, see
// <https://www.gnu.org/licenses/gpl.html>

#include <papoon.hxx>

#include <stm32f103xb.hxx>


namespace stm32f10_12357_xx {

using namespace stm32f103xb;


void usb_mcu_init()
{
#ifdef USB_DEV_FLASH_WAIT_STATES
#if USB_DEV_FLASH_WAIT_STATES == 1
    // enable flash prefetch buffer, one wait state
    flash->acr |= Flash::Acr::PRFTBE | Flash::Acr::LATENCY_1_WAIT_STATE;
#elif USB_DEV_FLASH_WAIT_STATES == 2
    // enable flash prefetch buffer, two wait states
    flash->acr |= Flash::Acr::PRFTBE | Flash::Acr::LATENCY_2_WAIT_STATES;
#endif
#endif

    // TJT is curious
    // printf ( "TJT PERIPH_BASE = %08x\n", PERIPH_BASE );
    // printf ( "TJT (addr) PERIPH_BASE = %08x\n", &PERIPH_BASE );

    rcc->cr |= Rcc::Cr::HSEON;
    while(!rcc->cr.any(Rcc::Cr::HSERDY));

    rcc->cfgr |=   Rcc::Cfgr::HPRE_DIV_1
                 | Rcc::Cfgr::PPRE2_DIV_1
                 | Rcc::Cfgr::PPRE1_DIV_1;

    rcc->cfgr.ins(  Rcc::Cfgr::PLLSRC     // PLL input from HSE (8 MHz)
                  | Rcc::Cfgr::PLLMULL_9);// Multiply by 9 (8*9=72 MHz)

    rcc->cr |= Rcc::Cr::PLLON;
    while(!rcc->cr.any(Rcc::Cr::PLLRDY));

    rcc->cfgr.ins(Rcc::Cfgr::SW_PLL);           // use PLL as system clock
    while(!rcc->cfgr.all(Rcc::Cfgr::SWS_PLL));  // wait for confirmation

    rcc->cfgr.clr(Rcc::Cfgr::USBPRE);           // 1.5x USB prescaler

    rcc->apb1enr |= Rcc::Apb1enr::USBEN;        // enable USB peripheral

#ifdef USB_DEV_DMA_PMA
    rcc->ahbenr |= Rcc::Ahbenr::DMA1EN;
#endif

    // enable GPIOs and alternate functions
    rcc->apb2enr |=   Rcc::Apb2enr::IOPAEN
                    | Rcc::Apb2enr::IOPCEN
                    | Rcc::Apb2enr::AFIOEN;

}  // usb_mcu_init()



void usb_gpio_init()
{
    // set user LED to output, 2 MHz
    gpioc->crh.ins(  Gpio::Crh::CNF13_OUTPUT_OPEN_DRAIN
                   | Gpio::Crh::MODE13_OUTPUT_2_MHZ    );

    // enable USB alternate function on USB+ and USB- data pins
    // alternate function output, open-drain (for paranoia), speed 50MHz
    // == 0b1111
    // some parameters reset by USB peripheral anyway??
    gpioa->crh.ins(  Gpio::Crh::CNF11_ALTFUNC_OPEN_DRAIN
                   | Gpio::Crh::CNF12_ALTFUNC_OPEN_DRAIN
                   | Gpio::Crh::MODE11_OUTPUT_50_MHZ
                   | Gpio::Crh::MODE12_OUTPUT_50_MHZ     );
}

}  // namespace stm32f10_12357_xx
