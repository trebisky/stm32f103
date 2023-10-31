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
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

OBJS = locore.o main.o nvic.o rcc.o gpio.o prf.o serial.o timer.o usb.o

all: tiny.elf tiny.dump

tiny.dump:	tiny.elf
	$(DUMP) tiny.elf >tiny.dump

tiny.bin:	tiny.elf
	$(OBJCOPY) tiny.elf tiny.bin -O binary

tiny.elf: 	$(OBJS)
	$(LD) -T simple.lds -o tiny.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	tiny.elf
	openocd $(OCDCFG) -c "program tiny.elf verify reset exit"

# No need for this now that the above works
bflash:	tiny.bin
	openocd $(OCDCFG) -c "program tiny.bin verify reset exit 0x08000000"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" tiny.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" tiny.elf

clean:
	rm -f *.o tiny.elf tiny.bin tiny.dump
