# Makefile for tidy blink demo

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as
# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc -mcpu=cortex-m3 -mthumb -g 
#CC = $(TOOLS)-gcc -mcpu=cortex-m3 -mthumb
CC = $(TOOLS)-gcc -mcpu=cortex-m3 -mthumb -O2

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

OBJS = locore.o blink.o rcc.o gpio.o

all: blink.elf blink.dump

blink.dump:	blink.elf
	$(DUMP) blink.elf >blink.dump

blink.bin:	blink.elf
	$(OBJCOPY) blink.elf blink.bin -O binary

blink.elf: 	$(OBJS)
	$(LD) -T blink.lds -o blink.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

# This is what we expect
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
# Some newer chips are shipping with a idcode of 0x2ba01477, this will make them work.
#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	blink.elf
	openocd $(OCDCFG) -c "program blink.elf verify reset exit"

# No need for this now that the above works
bflash:	blink.bin
	openocd $(OCDCFG) -c "program blink.bin verify reset exit 0x08000000"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" blink.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" blink.elf

clean:
	rm -f *.o blink.elf blink.bin blink.dump
