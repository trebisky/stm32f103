# Makefile for memory experiments

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

OBJS = locore.o startup.o mem.o rcc.o gpio.o serial.o timer.o

all: mem.elf mem.dump

mem.dump:	mem.elf
	$(DUMP) mem.elf >mem.dump

mem.bin:	mem.elf
	$(OBJCOPY) mem.elf mem.bin -O binary

mem.elf: 	$(OBJS)
	$(LD) -T mem.lds -o mem.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	mem.elf
	openocd $(OCDCFG) -c "program mem.elf verify reset exit"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" mem.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" mem.elf

clean:
	rm -f *.o mem.elf mem.bin mem.dump
