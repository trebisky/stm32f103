This is a collection of things I have done pertaining to the STM32F103
microcontroller.  I program using the C language and Makefiles, so this
is a collection of "on the bare metal" style stuff.  By and large it is
the record of my education on these devices.

My most recent work has moved to these projects:

https://github.com/trebisky/stm32f411

https://github.com/trebisky/libmaple-unwired

It all began when I ordered a couple of
"STM32F103C8T6 ARM STM32 Minimum System Development Board"
from AliExpress for $2.00.  The are now selling for less than that,
and with free shipping.  These are also known as the "blue pill"
and are discussed in the STM32duino forums.

See also the link above to the STM32F411 based "black pill" that
I am now starting to work with in a similar fashion.

For lots of notes and information, see my website:

http://cholla.mmto.org/stm32/

Along the way I ran into David Welch who gave me some valuable pointers
and hearty encouragment.  He is doing the same sorts of things with the
same gadgets:

https://github.com/dwelch67/stm32_samples/tree/master/STM32F103C8T6

Note that many of the early examples have a naive linker script
(lds file) which does not initialize BSS or global variables at all.
Later examples require this.  The presence of startup.c should be
an indicator of more sophisticated and proper treatement.

***

Some of these projects target a Maple board (or Olimexduino).
These projects are linked to and loaded at a different address
(0x08005000 rather than 0x08000000).  This is because these
boards have a USB laoder in the start of flash and I don't
want to overwrite it.  In fact I just go ahead and use it
instead of using SWD as I do for the blue pill.

***

Many of these projects require/expect a serial port.

This requires a USB to serial gadget connected to PB9 and PB10
Connect PA9 to RX
Connect PA10 to TX
also connect ground.

***

Here is a list of projects from 2017 ---

1. loader - host side program to talk to serial bootloader.
1. serial_boot - disassembly of the boot loader

1. blink1 - very basic LED blinking
2. blink1b - minor code cleanup on the above
3. blink_ext - blink external LED on A0
4. blink2 - more code reorganization and file grouping
5. blink_maple - blink LED on PA5 rather than PC13  --M
6. interrupt - do timer interrupts
6. lcd - drive a 16x2 LCD module using i2c

6. memory - figure out how to handle bss and initialized data --B
7. i2c_maple - use libmaple for i2c DAC --B
8. adc_serial - work with F103 adc --P

9. serial1 - first working serial (uart) driver
9. lithium1 - lithium ion battery tester --P

9. usb1 - get barely started working with USB --P


Note that blink_maple is the only demo built to run on the Maple board.

The letters are: M = links for Maple, B = zeros BSS, P = includes printf

***

The above work was done in 2017, I am beginning new work in 2023,
focusing on learning about low level USB

1. usb2 - a copy of usb1 with new fixes  --P
2. usb_papoon - the papoon_usb framework and a working echo! -BP
3. usb_papoon2 - the papoon_usb framework using interrupts -BP

