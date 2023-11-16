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


#ifndef USB_MCY_INIT_HXX
#define USB_MCY_INIT_HXX

namespace stm32f10_12357_xx {

void usb_mcu_init ();
void usb_gpio_init();

}  // namespace stm32f10_12357_xx

#endif   // ifndef USB_MCY_INIT_HXX
