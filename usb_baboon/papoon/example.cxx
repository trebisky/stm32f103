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

#include <papoon.h>

// #include <core_cm3.hxx>
// #include <stm32f103xb.hxx>

#include <usb_dev_cdc_acm.h>

// #include <usb_mcu_init.h>

using namespace stm32f10_12357_xx;

UsbDevCdcAcm    usb_dev;

uint8_t         recv_buf[UsbDevCdcAcm::CDC_OUT_DATA_SIZE],
                send_buf[UsbDevCdcAcm::CDC_IN_DATA_SIZE ];

extern "C" void
papoon_handler ( void )
{
	usb_dev.interrupt_handler();
}

// int main()
extern "C" void
papoon_init ( void )
{
    // nonsense
    // usb_dev.serial_number_init();

    // usb_mcu_init();

    usb_dev.init();

#ifdef USB_POLL
    printf ( "Waiting for papoon init\n" );
    while (usb_dev.device_state() != UsbDev::DeviceState::CONFIGURED)
        usb_dev.poll();
#endif

    // papoon_wait ( 8 );

    printf ( "Papoon initialization done\n" );
    // printf ( "  state = %d\n", usb_dev.device_state() );
}

extern "C" int
is_papoon_configured ( void )
{
    if (usb_dev.device_state() == UsbDev::DeviceState::CONFIGURED)
	return 1;
    else
	return 0;
}

extern "C" void
papoon_wait ( int sec )
{
	int tmo = sec * 1000;

	while ( tmo-- ) {
	    delay_ms ( 1 );
	    if ( usb_dev.device_state() == UsbDev::DeviceState::CONFIGURED ) {
		printf ( " Wait for enumeration suceeded\n" );
		printf ( "  state = %d\n", usb_dev.device_state() );
		return;
	    }
	}
	printf ( " Wait for enumeration timed out\n" );
	printf ( "  state = %d\n", usb_dev.device_state() );
}


int tjt_debug = 0;

extern "C" void
papoon_debug ( void )
{
	tjt_debug = 1;
}

// XXX break into 64 byte chunks
extern "C" void
papoon_send ( const uint8_t *buf, int len )
{
	printf ( "Papoon send %d: %s\n", len, buf );
	while ( ! usb_dev.send ( UsbDevCdcAcm::CDC_ENDPOINT_IN, buf, len ) )
	    ;
}

/* Here we see the "56" bug -- or something worse!! */
extern "C" void
papoon_putc ( const uint8_t cc )
{
	// This doesn't work at all !!!
	usb_dev.putc ( UsbDevCdcAcm::CDC_ENDPOINT_IN, cc );

	// The following yields 56 bug
	// usb_dev.send ( UsbDevCdcAcm::CDC_ENDPOINT_IN, &cc, 1 );
}

/* Note that send() should never be called with more than 64 bytes
 * (the size of the endpoint buffer).
 * Similarly don't expect more than this from recv().
 */
extern "C" void
papoon_demo ( void )
{
    printf ( "Papoon demo starting\n" );
    while (true) {
        uint16_t    recv_len,
                    send_len;

	int i;
	int j;

        if (recv_len = usb_dev.recv(UsbDevCdcAcm::CDC_ENDPOINT_OUT, recv_buf)) {

            // process data received from host -- populate send_buf and set send_len
	    // printf ( "Papoon recv %d\n", recv_len );
	    // we see single characters received as we type on picocom
	    j = 0;
	    for ( i=0; i<recv_len; i++ ) {
		send_buf[j++] = recv_buf[i];
		// This doubles all chars
		// send_buf[j++] = recv_buf[i];
	    }
	    send_len = j;

            while (!usb_dev.send(UsbDevCdcAcm::CDC_ENDPOINT_IN, send_buf, send_len))
                ;
        }
    }
}

#ifdef USB_POLL
extern "C" void
papoon_demo ( void )
{
    while (true) {
        usb_dev.poll();

        uint16_t    recv_len,
                    send_len;

	int i;
	int j;

        if (recv_len = usb_dev.recv(UsbDevCdcAcm::CDC_ENDPOINT_OUT, recv_buf)) {

            // process data received from host -- populate send_buf and set send_len
	    // printf ( "Papoon recv %d\n", recv_len );
	    // we see single characters received as we type on picocom
	    j = 0;
	    for ( i=0; i<recv_len; i++ ) {
		send_buf[j++] = recv_buf[i];
		// This doubles all chars
		// send_buf[j++] = recv_buf[i];
	    }
	    send_len = j;

            while (!usb_dev.send(UsbDevCdcAcm::CDC_ENDPOINT_IN, send_buf, send_len))
                usb_dev.poll();
        }
    }
}
#endif

/* THE END */
