# Makefile for blink demo

AS = arm-linux-gnu-as
CC = arm-linux-gnu-gcc
#LD = arm-linux-gnu-gcc
LD = arm-linux-gnu-ld.bfd
OBJCOPY = arm-linux-gnu-objcopy

OBJS = locore.o blink.o

all: blink.bin

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


clean:
	rm -f *.o blink.elf blink.bin
