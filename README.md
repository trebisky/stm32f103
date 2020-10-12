This is a collection of things I have done pertaining to the STM32F103
microcontroller.  I program using the C language and Makefiles, so this
is a collection of "on the bare metal" style stuff.  By and large it is
the record of my education on these devices.

My most recent work has moved to this project:

https://github.com/trebisky/libmaple-unwired

It all began when I ordered a couple of
"STM32F103C8T6 ARM STM32 Minimum System Development Board"
from AliExpress for $2.00.  The are now selling for less than that,
and with free shipping.  These are also known as the "blue pill"
and are discussed in the STM32duino forums.

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
