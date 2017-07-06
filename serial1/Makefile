# Makefile for talk serial demo

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as

COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration
# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc $(COPTS) -g 
#CC = $(TOOLS)-gcc $(COPTS)
CC = $(TOOLS)-gcc $(COPTS) -O2

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

OBJS = locore.o talk.o rcc.o gpio.o serial.o timer.o

all: talk.elf talk.dump

talk.dump:	talk.elf
	$(DUMP) talk.elf >talk.dump

talk.bin:	talk.elf
	$(OBJCOPY) talk.elf talk.bin -O binary

talk.elf: 	$(OBJS)
	$(LD) -T talk.lds -o talk.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	talk.elf
	openocd $(OCDCFG) -c "program talk.elf verify reset exit"

# No need for this now that the above works
bflash:	talk.bin
	openocd $(OCDCFG) -c "program talk.bin verify reset exit 0x08000000"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" talk.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" talk.elf

clean:
	rm -f *.o talk.elf talk.bin talk.dump
