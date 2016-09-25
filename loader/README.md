This is a C program to talk to the serial bootloader on the STM32F103 chip.

At this time it does only one thing, namely sends the "read unprotect" command,
which I find essential to unlock the chips I have received.

I expect to develop it into a more general tool.
