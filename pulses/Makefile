# Makefile for pulses demo
#
# I began with my super simple "blink1" demo
# I need a 1 Hz square wave, and this seemed
# like a really quick and easy way to get one.
#
# 10-16-2026

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as
# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc -mcpu=cortex-m3 -mthumb
CC = $(TOOLS)-gcc -mcpu=cortex-m3 -mthumb -g

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

OBJS = pulses.o locore.o

all: pulses.bin pulses.dump

pulses.dump:	pulses.elf
	$(DUMP) pulses.elf >pulses.dump

pulses.bin:	pulses.elf
	$(OBJCOPY) pulses.elf pulses.bin -O binary

pulses.elf: 	$(OBJS)
	$(LD) -T pulses.lds -o pulses.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

pulses.o:	pulses.c
	$(CC) -c pulses.c

# This is what we expect
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
# Some newer chips are shipping with a idcode of 0x2ba01477, this will make them work.
#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg

flash:  pulses.elf
	openocd $(OCDCFG) -c "program pulses.elf verify reset exit"

new_flash:
	st-flash write pulses.bin 0x8000000

# This is how I first learned to burn the image.
#  (I use the above "make flash" these days.
#
#  Connect the STLINK gadget to the target.
#  Be sure and start openOCD in this directory.
#    (or else it won't be able to find the bin file).
#  Then use "make flash" to start an openocd console session.
#  Type: reset halt
#  Type: flash write_image erase pulses.bin 0x08000000
old_flash:
	@echo "type: flash write_image erase pulses.bin 0x08000000"
	telnet localhost 4444
gdb:
	$(GDB) --eval-command="target remote localhost:3333" pulses.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" pulses.elf

clean:
	rm -f *.o pulses.elf pulses.bin pulses.dump
