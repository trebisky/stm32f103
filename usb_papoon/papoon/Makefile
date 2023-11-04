# Makefile for papoon submodule

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

OBJS = example.o usb_dev.o usb_dev_cdc_acm.o usb_mcu_init.o

all: ../papoon.o

../papoon.o: $(OBJS)
	$(LD) -r -o ../papoon.o $(OBJS)

# ===========================================
# c++ compiler setup from Papoon
#$(CXX) -c $(CXX_FLAGS) $(INCLUDE_DIRS) $(INCLUDES) $(CONFIGURATION)
#%.o: %.cxx
#        $(CXX) -c $(CXX_FLAGS) $(INCLUDE_DIRS) $(INCLUDES) $(CONFIGURATION) \
#               $<  -o $@

CXX = $(TOOLS)-g++

# Without the -fno-exceptions, we get calls to:
# __aeabi_unwind_cpp_pr0
# __aeabi_unwind_cpp_pr1

CXX_FLAGS = -g -O1 -mcpu=cortex-m3 -mthumb -fno-builtin -fno-exceptions
INCLUDES = -I.
SEND_B4_RECV    ?= 64
SYNC_LEN  	?= 4
REPORT_EVERY    ?= 10000
ASYNC           ?= -D
DEBUG           ?= -U

CONFIGURATION = -DUP_MAX_PACKET_SIZE=64                 \
                -DDOWN_MAX_PACKET_SIZE=64               \
                -DSEND_B4_RECV_MAX=$(SEND_B4_RECV)      \
                -DUSB_RANDOMTEST_UP_SEED=0x5f443bba     \
                -DUSB_RANDOMTEST_DOWN_SEED=0x684053d8   \
                -DUSB_RANDOMTEST_LENGTH_SEED=0x769bc5e6 \
                -DSLAVE_LENGTH_SEED=0x4b6420e0          \
                -DUSB_RANDOMTEST_SYNC_LENGTH=$(SYNC_LEN)\
                -DHISTOGRAM_LENGTH=8                    \
                -DREPORT_EVERY=$(REPORT_EVERY)          \
                $(ASYNC)RANDOMTEST_LIBUSB_ASYNC         \
                $(DEBUG)DEBUG


CXX_OPTIONS = $(CXX_FLAGS) $(INCLUDES) $(CONFIGURATION)


%.o: %.cxx
	$(CXX) -c $(CXX_OPTIONS) $< -o $@

# Copy files from the pappon project
PAPOON_PATH = /u1/Projects/STM32/papoon_usb
get:
	cp $(PAPOON_PATH)/examples/blue_pill/example.cxx .
	cp $(PAPOON_PATH)/arm/core_cm3.hxx .
	cp $(PAPOON_PATH)/regbits/regbits.hxx .
	cp $(PAPOON_PATH)/regbits/stm32f103xb.hxx .
	cp $(PAPOON_PATH)/regbits/stm32f103xb_tim.hxx .
	cp $(PAPOON_PATH)/usb/usb_dev_cdc_acm.hxx .
	cp $(PAPOON_PATH)/usb/usb_dev.hxx .
	cp $(PAPOON_PATH)/examples/blue_pill/usb_mcu_init.hxx .
	cp $(PAPOON_PATH)/usb/usb_dev.cxx .
	cp $(PAPOON_PATH)/util/bin_to_hex.hxx .
	cp $(PAPOON_PATH)/usb/usb_dev_cdc_acm.cxx .
	cp $(PAPOON_PATH)/examples/blue_pill/usb_mcu_init.cxx .

clean:
	rm -f *.o


# papoon.dump:	papoon.elf
# 	$(DUMP) papoon.elf >papoon.dump
# 
# papoon.bin:	papoon.elf
# 	$(OBJCOPY) papoon.elf papoon.bin -O binary
# 
# papoon.elf: 	$(OBJS)
# 	$(LD) -T simple.lds -o papoon.elf $(OBJS)
# 
# locore.o:	locore.s
# 	$(AS) locore.s -o locore.o
# 
# .c.o:
# 	$(CC) -o $@ -c $<
# 
# OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
# 
# # This works great once I fixed the base address in the LDS file
# flash:	papoon.elf
# 	openocd $(OCDCFG) -c "program papoon.elf verify reset exit"
# 
# # No need for this now that the above works
# bflash:	papoon.bin
# 	openocd $(OCDCFG) -c "program papoon.bin verify reset exit 0x08000000"
# 
# gdb:
# 	$(GDB) --eval-command="target remote localhost:3333" papoon.elf
# 
# gdbtui:
# 	$(GDB) -tui --eval-command="target remote localhost:3333" papoon.elf
# 
# clean:
# 	rm -f *.o papoon.elf papoon.bin papoon.dump
