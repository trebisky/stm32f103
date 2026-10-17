# Makefile to archive maple loader

# XXX this is just the makefile for usb_baboon and does NOT
# have the commands used to obtain the loader.

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
