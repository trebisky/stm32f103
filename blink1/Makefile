# Makefile for blink demo

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

AS = $(TOOLS)-as
CC = $(TOOLS)-gcc
#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d

OBJS = locore.o blink.o

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

#        $(LD) -g -T kyu.lds -e kern_startup -o kyu $(OBJS) $(LIBS)
# -nostdlib -nostartfiles -ffreestanding

gdb:
	arm-none-eabi-gdb --eval-command="target remote localhost:3333" blink.elf

clean:
	rm -f *.o blink.elf blink.bin
