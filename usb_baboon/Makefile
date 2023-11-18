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

all: baboon.elf baboon.dump tags

papoon.o:	bogus
	cd papoon; make

bogus:

baboon.dump:	baboon.elf
	$(DUMP) baboon.elf >baboon.dump

baboon.bin:	baboon.elf
	$(OBJCOPY) baboon.elf baboon.bin -O binary

baboon.elf: 	$(OBJS)
	$(LD) -T baboon.lds -o baboon.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# Does not work yet for the Maple
# reset takes the CPU to 08000000 and either nothing is there
# or it runs an old blink demo it finds there.
flash:	baboon.elf
	openocd $(OCDCFG) -c "program baboon.elf verify reset exit"

# No need for this now that the above works
##bflash:	baboon.bin
##	openocd $(OCDCFG) -c "program baboon.bin verify reset exit 0x08000000"

xyz:
	openocd $(OCDCFG)

ocd:
	openocd $(OCDCFG)

gdb:
	$(GDB) --eval-command="target remote localhost:3333" baboon.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" baboon.elf

tags:
	ctags -R .

clean:
	cd papoon; make clean
	rm -f *.o baboon.elf baboon.bin baboon.dump
	rm -f tags
