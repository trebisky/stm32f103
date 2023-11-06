# Makefile for interrupt demo

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as

#COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration
COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -fno-builtin-printf -fno-builtin
# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc $(COPTS) -g 
#CC = $(TOOLS)-gcc $(COPTS)
CC = $(TOOLS)-gcc $(COPTS) -O2

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd --no-warn-rwx-segments
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

#OBJS = locore.o main.o nvic.o rcc.o gpio.o prf.o serial.o timer.o usb.o
OBJS = locore.o main.o startup.o nvic.o rcc.o gpio.o prf.o serial.o timer.o usb.o papoon.o

all: papoon.elf papoon.dump

papoon.o:	bogus
	cd papoon; make

bogus:

papoon.dump:	papoon.elf
	$(DUMP) papoon.elf >papoon.dump

papoon.bin:	papoon.elf
	$(OBJCOPY) papoon.elf papoon.bin -O binary

papoon.elf: 	$(OBJS)
	$(LD) -T papoon.lds -o papoon.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	papoon.elf
	openocd $(OCDCFG) -c "program papoon.elf verify reset exit"

# No need for this now that the above works
bflash:	papoon.bin
	openocd $(OCDCFG) -c "program papoon.bin verify reset exit 0x08000000"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" papoon.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" papoon.elf

tags:
	ctags -R .

clean:
	cd papoon; make clean
	rm -f *.o papoon.elf papoon.bin papoon.dump
	rm -f tags
