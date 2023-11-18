# Makefile for papoon submodule

#TOOLS = arm-linux-gnu
TOOLS = arm-none-eabi

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd --no-warn-rwx-segments

OBJS = example.o usb_dev.o usb_dev_cdc_acm.o usb_mcu_init.o

all: ../papoon.o

../papoon.o: $(OBJS)
	$(LD) -r -o ../papoon.o $(OBJS)

# ===========================================

CXX = $(TOOLS)-g++

# Without the -fno-exceptions, we get calls to:
# __aeabi_unwind_cpp_pr0
# __aeabi_unwind_cpp_pr1

#CXX_FLAGS = -g -O1 -mcpu=cortex-m3 -mthumb -fno-builtin -fno-exceptions
CXX_FLAGS = -O1 -mcpu=cortex-m3 -mthumb -fno-builtin -fno-exceptions
INCLUDES = -I.

# All this is now in papoon.hxx
#SEND_B4_RECV    ?= 64
#SYNC_LEN  	?= 4
#REPORT_EVERY    ?= 10000
#ASYNC           ?= -D
#DEBUG           ?= -U
#
#CONFIGURATION = -DUP_MAX_PACKET_SIZE=64                 \
#                -DDOWN_MAX_PACKET_SIZE=64               \
#                -DSEND_B4_RECV_MAX=$(SEND_B4_RECV)      \
#                -DUSB_RANDOMTEST_UP_SEED=0x5f443bba     \
#                -DUSB_RANDOMTEST_DOWN_SEED=0x684053d8   \
#                -DUSB_RANDOMTEST_LENGTH_SEED=0x769bc5e6 \
#                -DSLAVE_LENGTH_SEED=0x4b6420e0          \
#                -DUSB_RANDOMTEST_SYNC_LENGTH=$(SYNC_LEN)\
#                -DHISTOGRAM_LENGTH=8                    \
#                -DREPORT_EVERY=$(REPORT_EVERY)          \
#                $(ASYNC)RANDOMTEST_LIBUSB_ASYNC         \
#                $(DEBUG)DEBUG

#CXX_OPTIONS = $(CXX_FLAGS) $(INCLUDES) $(CONFIGURATION)
CXX_OPTIONS = $(CXX_FLAGS) $(INCLUDES)

%.o: %.cxx
	$(CXX) -c $(CXX_OPTIONS) $< -o $@

# Copy files from the papoon_usb project
# We did this to first copy the files, but now we have
# made changes to a few of them and if we did "make get"
# again, we would overwrite those changes, so we
# comment this out to avoid such a catastrophe.

PAPOON_PATH = /u1/Projects/STM32/papoon_usb
#get:
#	cp $(PAPOON_PATH)/examples/blue_pill/example.cxx .
#	cp $(PAPOON_PATH)/arm/core_cm3.hxx .
#	cp $(PAPOON_PATH)/regbits/regbits.hxx .
#	cp $(PAPOON_PATH)/regbits/stm32f103xb.hxx .
#	cp $(PAPOON_PATH)/regbits/stm32f103xb_tim.hxx .
#	cp $(PAPOON_PATH)/usb/usb_dev_cdc_acm.hxx .
#	cp $(PAPOON_PATH)/usb/usb_dev.hxx .
#	cp $(PAPOON_PATH)/examples/blue_pill/usb_mcu_init.hxx .
#	cp $(PAPOON_PATH)/usb/usb_dev.cxx .
#	cp $(PAPOON_PATH)/util/bin_to_hex.hxx .
#	cp $(PAPOON_PATH)/usb/usb_dev_cdc_acm.cxx .
#	cp $(PAPOON_PATH)/examples/blue_pill/usb_mcu_init.cxx .

clean:
	rm -f *.o

# THE END
