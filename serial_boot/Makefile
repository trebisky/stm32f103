# Makefile for serial bootrom disassembly
# Tom Trebisky  9-25-2016

TOOLS = arm-none-eabi

# DUMP = $(TOOLS)-objdump -D -b binary -marm
# -mcpu=cortex-m3 -mthumb
#-marm flash_orig_dump.bin -Mforce-thumb

BDUMP = $(TOOLS)-objdump -D -b binary -marm -Mforce-thumb

all:	boot.odx

naive.dis:	serialboot.bin
	$(BDUMP) serialboot.bin >naive.dis

naive2.dis:	serialboot.bin
	$(BDUMP) --start-address=0x20 serialboot.bin >naive2.dis

x.dis:	serialboot.bin
	$(BDUMP) --start-address=0x10 serialboot.bin >x.dis

reloc.dis:	naive2.dis
	./reloc >reloc.dis

boot.odx:	serialboot.bin
	odx serialboot.bin >boot.odx

# Does not work
#EDUMP = $(TOOLS)-objdump -D -marmv5 -Mforce-thumb
#boot.dis:	boot.elf
#	$(EDUMP) --start-address=0x1ffff020 boot.elf >boot.dis
	
boot.elf:	serialboot.bin wrap
	./wrap serialboot.bin boot.elf

longs.out:
	./longs serialboot.bin >longs.out

#wrap:	wrap.c
#	cc -o wrap wrap.c

GDB = $(TOOLS)-gdb
gdb:
	$(GDB) --eval-command="target remote localhost:3333" blink.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" blink.elf

clean:
	rm -f *.dis wrap longs.out boot.odx
	rm -r boot.elf
