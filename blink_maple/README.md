This is just the blink2 demo copied here and fiddled with
to run on a Maple r5 board.

The lds file must be changed so the code starts at 0x08005000

Also the LED is now PA5

Note that the GPIO is hard wired in gpio.c

Also, to load this (as intended) via the Maple DFU bootloader,
you will probably need to put the board into "perpetual boot mode".
Hit reset, then as soon as the fast blinks start, hold down the
other button a few seconds.
