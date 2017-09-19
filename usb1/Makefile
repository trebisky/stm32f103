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

OBJS = locore.o usb1_main.o nvic.o rcc.o gpio.o prf.o serial.o timer.o usb.o

all: usb1.elf usb1.dump

usb1.dump:	usb1.elf
	$(DUMP) usb1.elf >usb1.dump

usb1.bin:	usb1.elf
	$(OBJCOPY) usb1.elf usb1.bin -O binary

usb1.elf: 	$(OBJS)
	$(LD) -T usb1.lds -o usb1.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:	usb1.elf
	openocd $(OCDCFG) -c "program usb1.elf verify reset exit"

welch:	notmain.elf
	openocd $(OCDCFG) -c "program notmain.elf verify reset exit"

# No need for this now that the above works
bflash:	usb1.bin
	openocd $(OCDCFG) -c "program usb1.bin verify reset exit 0x08000000"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" usb1.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" usb1.elf

clean:
	rm -f *.o usb1.elf usb1.bin usb1.dump
