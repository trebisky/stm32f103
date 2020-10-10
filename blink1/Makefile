# Makefile for blink demo
#
# This is/was the first code I ever wrote to
# run on an STM32F103.

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

#OBJS = locore.o blink.o
OBJS = blink.o locore.o

all: blink.bin blink.dump

blink.dump:	blink.elf
	$(DUMP) blink.elf >blink.dump

blink.bin:	blink.elf
	$(OBJCOPY) blink.elf blink.bin -O binary

blink.elf: 	$(OBJS)
	$(LD) -T blink.lds -o blink.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

blink.o:	blink.c
	$(CC) -c blink.c

# This is what we expect
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
# Some newer chips are shipping with a idcode of 0x2ba01477, this will make them work.
#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg

flash:  blink.elf
	openocd $(OCDCFG) -c "program blink.elf verify reset exit"

# This is how I first learned to burn the image.
#  (I use the above "make flash" these days.
#
#  Connect the STLINK gadget to the target.
#  Be sure and start openOCD in this directory.
#    (or else it won't be able to find the bin file).
#  Then use "make flash" to start an openocd console session.
#  Type: reset halt
#  Type: flash write_image erase blink.bin 0x08000000
old_flash:
	@echo "type: flash write_image erase blink.bin 0x08000000"
	telnet localhost 4444
gdb:
	$(GDB) --eval-command="target remote localhost:3333" blink.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" blink.elf

clean:
	rm -f *.o blink.elf blink.bin blink.dump
