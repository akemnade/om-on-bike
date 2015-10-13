AS = gpasm
CFLAGS= $(MORE_CFLAGS)
CPU = 18f26j50
# --use-non-free enables use of register definitions which are
# available with every compiler
CC = sdcc -mpic16 -p$(CPU) -Wl,-s -Wl,$(CPU).lkr --use-non-free  
all: main.hex

.SUFFIXES: .c .o .asm
program: main.stamp
	 p18f-bootloader-libusb bt:$(DEST) power on 

# a single goto in case no bootloader is used and config flags
e1.o: e1.asm
quickread.o: quickread.asm
serirq.o: serirq.asm


sdcard.o: sdcard.c sdcard.h
main.o: main.c usb-drv.h i2cif.h sdcard.h
usb-drv.c: usb-drv.h dev-descs.h
i2cif.c: i2cif.h

OBJS = main.o e1.o usb-drv.o i2cif.o sdcard.o quickread.o serirq.o

main.hex: $(OBJS)
	$(CC) $(OBJS) -llibc18f.lib	

main.stamp: main.hex
	p18f-bootloader-libusb bt:x  progverify main.hex && touch main.stamp

clean:
	rm main.stamp *.o main.hex sdcard.asm usb-drv.asm i2cif.asm main.asm

.PHONY: clean
.asm.o:
	$(AS) -c $<

