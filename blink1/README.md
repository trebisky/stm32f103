Here it is!  My first code ever to run on the ARM Cortex M3,
in this case the STM32F103C8.  This just blinks the LED that
is wired up to GPIO pin PC13.  

It is a "proof of concept" milestone.  The smallest bit of
C and assembly that will make the device do something
recognizable.  I flashed the code using an STLINK V2 and
OpenOCD under linux.

It should be instructive to someone wanting to see a truly
minimal bit of code along with a Makefile.

-----------------------------------

And 3 months later I find myself trying to remember how to do
this (namely flash the image).  Using Fedora 25

dnf install arm-none-eabi-gcc-cs

dnf install openocd

make clean

make

then

Connect the ST link to the STM32 unit.

    This supplies power

Then I open up two extra terminal windows.

In one I run openocd by typing:

    cd /u1/Projects/STM32/Archive/blink1

    ./ocd

In the other I type "telnet localhost 4444"

Then in the telnet window I give two commands to openocd

    reset halt

    flash write_image erase blink.bin 0x08000000

Then I press the reset button and voila, flashing starts.

Note that openocd must be running in the directory that
holds the bin file or it won't be able to find it.
It doesn't matter what directory telnet runs in.
