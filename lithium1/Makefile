# Makefile for interrupt demo

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as

#COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration
COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -fno-builtin-printf
# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc $(COPTS) -g 
#CC = $(TOOLS)-gcc $(COPTS)
CC = $(TOOLS)-gcc $(COPTS) -O2

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

OBJS = locore.o lithium.o nvic.o rcc.o gpio.o prf.o serial.o timer.o adc.o

all: lithium.elf lithium.dump

lithium.dump:	lithium.elf
	$(DUMP) lithium.elf >lithium.dump

lithium.bin:	lithium.elf
	$(OBJCOPY) lithium.elf lithium.bin -O binary

lithium.elf: 	$(OBJS)
	$(LD) -T lithium.lds -o lithium.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	lithium.elf
	openocd $(OCDCFG) -c "program lithium.elf verify reset exit"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" lithium.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" lithium.elf

clean:
	rm -f *.o lithium.elf lithium.bin lithium.dump
