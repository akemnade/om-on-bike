AS = gpasm
CFLAGS= $(MORE_CFLAGS)
# --use-non-free enables use of register definitions which are
# available with every compiler
CC = sdcc -mpic16 -p18f25j50 -Wl,-s -Wl,18f25j50b.lkr --use-non-free  
all: main.hex
	 p18f-bootloader-libusb bt:$(DEST) power on 

program: main.stamp

# a single goto in case no bootloader is used and config flags
e1.o: e1.asm
	$(AS) -c e1.asm
main.o: main.c usb-drv.h i2cif.h
usb-drv.c: usb-drv.h dev-descs.h
i2cif.c: i2cif.h 
main.hex: main.o e1.o usb-drv.o i2cif.o
	$(CC) main.o e1.o usb-drv.o i2cif.o -llibc18f.lib	

main.stamp: main.hex
	p18f-bootloader-libusb bt:x  progverify main.hex && touch main.stamp

clean:
	rm main.stamp leds.o main.o main.hex
