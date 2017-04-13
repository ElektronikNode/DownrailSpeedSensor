OOCD = openocd
OOCDFLAGS = -f interface/ftdi/olimex-arm-usb-ocd-h.cfg -f target/stm32f1x.cfg

include Makefile.chibios

program: $(BUILDDIR)/$(PROJECT).elf
	$(OOCD) $(OOCDFLAGS) \
	  -c "init" -c "targets" -c "reset halt" \
	  -c "flash write_image erase $<" -c "verify_image $<" \
	  -c "reset run" -c "shutdown"
