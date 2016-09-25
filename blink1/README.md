Here it is!  My first code ever to run on the ARM Cortex M3,
in this case the STM32F103C8.  This just blinks the LED that
is wired up to GPIO pin PC13.  

It is a "proof of concept" milestone.  The smallest bit of
C and assembly that will make the device do something
recognizable.  I flashed the code using an STLINK V2 and
OpenOCD under linux.

It should be instructive to someone wanting to see a truly
minimal bit of code along with a Makefile.
