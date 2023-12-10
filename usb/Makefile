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

##-Wl,-gc-sections,-Map,main.map -Wl,--cref

#LDOPTS = --no-warn-rwx-segments
LDOPTS = -no-warn-rwx-segments -Map dragoon.map --cref

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd $(LDOPTS)
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

OBJS = locore.o main.o startup.o nvic.o rcc.o gpio.o prf.o kyulib.o serial.o timer.o usb.o usb_enum.o usb_watch.o

all: dragoon.elf dragoon.dump tags

dragoon.dump:	dragoon.elf
	$(DUMP) dragoon.elf >dragoon.dump

dragoon.bin:	dragoon.elf
	$(OBJCOPY) dragoon.elf dragoon.bin -O binary

dragoon.elf: 	$(OBJS)
	$(LD) -T dragoon.lds -o dragoon.elf $(OBJS)
	size dragoon.elf

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

# Does not work yet for the Maple
# reset takes the CPU to 08000000 and either nothing is there
# or it runs an old blink demo it finds there.
flash:	dragoon.elf
	openocd $(OCDCFG) -c "program dragoon.elf verify reset exit"

ocd:
	openocd $(OCDCFG)

gdb:
	$(GDB) --eval-command="target remote localhost:3333" dragoon.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" dragoon.elf

# This is useful when a vid/pid is selected that is not known by any standard
# driver.  I used it with the "daed" code -- and it is persistent until the
# system is rebooted.
usb-serial:
	echo 0483 5722 >/sys/bus/usb-serial/drivers/generic/new_id

tags:
	ctags -R .

clean:
	rm -f *.o dragoon.elf dragoon.bin dragoon.dump dragoon.map
	rm -f tags
