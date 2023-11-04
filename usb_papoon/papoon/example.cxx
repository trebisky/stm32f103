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


#include <core_cm3.hxx>
#include <stm32f103xb.hxx>

#include <usb_dev_cdc_acm.hxx>

#include <usb_mcu_init.hxx>


using namespace stm32f10_12357_xx;


UsbDevCdcAcm    usb_dev;

uint8_t         recv_buf[UsbDevCdcAcm::CDC_OUT_DATA_SIZE],
                send_buf[UsbDevCdcAcm::CDC_IN_DATA_SIZE ];

void cxx_main ( void );

extern "C" void
papoon_main ( void )
{
    cxx_main();
}

// int main()
void cxx_main()
{
    usb_dev.serial_number_init();

    usb_mcu_init();

    usb_dev.init();

    while (usb_dev.device_state() != UsbDev::DeviceState::CONFIGURED)
        usb_dev.poll();

    while (true) {
        usb_dev.poll();

        uint16_t    recv_len,
                    send_len;

        if (recv_len = usb_dev.recv(UsbDevCdcAcm::CDC_ENDPOINT_OUT, recv_buf)) {
            // process data received from host -- populate send_buf and set send_len

            while (!usb_dev.send(UsbDevCdcAcm::CDC_ENDPOINT_IN, send_buf, send_len))
                usb_dev.poll();
        }
    }
}
