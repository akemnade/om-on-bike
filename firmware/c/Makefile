AS = gpasm
CFLAGS= $(MORE_CFLAGS)
CPU = 18f26j50
# --use-non-free enables use of register definitions which are
# available with every compiler
CC = sdcc -mpic16 -p$(CPU) -Wl,-s -Wl,$(CPU).lkr --use-non-free  
all: main.hex

program: main.stamp
	 p18f-bootloader-libusb bt:$(DEST) power on 

# a single goto in case no bootloader is used and config flags
e1.o: e1.asm
	$(AS) -c e1.asm
serirq.o: serirq.asm
	$(AS) -c serirq.asm
sdcard.o: sdcard.c sdcard.h
main.o: main.c usb-drv.h i2cif.h sdcard.h
usb-drv.c: usb-drv.h dev-descs.h
i2cif.c: i2cif.h
main.hex: main.o e1.o usb-drv.o i2cif.o sdcard.o quickread.o serirq.o
	$(CC) main.o e1.o usb-drv.o i2cif.o sdcard.o quickread.o serirq.o -llibc18f.lib	

main.stamp: main.hex
	p18f-bootloader-libusb bt:x  progverify main.hex && touch main.stamp

clean:
	rm main.stamp *.o main.hex
