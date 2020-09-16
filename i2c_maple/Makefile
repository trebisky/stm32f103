# Makefile for i2c/dac demo using maple i2c driver

# From: libmaple/support/make/board-includes/maple_mini.mk
#MCU := STM32F103CB
#PRODUCT_ID := 0003
#ERROR_LED_PORT := GPIOB
#ERROR_LED_PIN  := 1
#MCU_SERIES := stm32f1
#MCU_F1_LINE := performance
#LD_MEM_DIR := sram_20k_flash_128k
# END

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as

#COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -I. -Ilibmaple -DMCU_STM32F103CB
COPTS = -mcpu=cortex-m3 -mthumb -Wno-implicit-function-declaration -I. -Ilibmaple -DMCU_STM32F103C8
# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc $(COPTS) -g 
#CC = $(TOOLS)-gcc $(COPTS)
CC = $(TOOLS)-gcc $(COPTS) -O2

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
OBJDUMP = $(TOOLS)-objdump
GDB = $(TOOLS)-gdb

#OBJS = locore.o dac.o nvic.o rcc.o gpio.o serial.o timer.o i2c.o
OBJS = locore.o startup.o dac.o nvic.o rcc.o gpio.o serial.o timer.o maple_i2c.o maple_stm32f103_i2c.o glue.o
#OBJS = locore.o dac.o nvic.o rcc.o gpio.o serial.o timer.o

all: dac.elf dac.dump

dac.dump:	dac.elf
	$(DUMP) dac.elf >dac.dump

dac.bin:	dac.elf
	$(OBJCOPY) dac.elf dac.bin -O binary

dac.elf: 	$(OBJS)
	$(LD) -T dac.lds -o dac.elf $(OBJS)

locore.o:	locore.s
	$(AS) locore.s -o locore.o

.c.o:
	$(CC) -o $@ -c $<

# This is what we expect
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
# Some newer chips are shipping with a idcode of 0x2ba01477, this will make them work.
#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg

# This works great once I fixed the base address in the LDS file
flash:  dac.elf
	openocd $(OCDCFG) -c "program dac.elf verify reset exit"

gdb:
	$(GDB) --eval-command="target remote localhost:3333" dac.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" dac.elf

hdr:
	$(OBJDUMP) -h maple_i2c.o

clean:
	rm -f *.o dac.elf dac.bin dac.dump
