CC=gcc
CFLAGS=-O2 -Wall -g
BINPROGS=gps-dr calibrate-compass read-tacho read-compass multistream merge-tacho-compass
all: $(BINPROGS)
calibrate-compass: calibrate-compass.o compass.o evt.o fxoss.o
	$(CC) -o calibrate-compass calibrate-compass.o compass.o evt.o fxoss.o -lpthread -lm
gps-dr:  multistream.o compass.o tacho.o evt.o fxoss.o
	$(CC) -o gps-dr  multistream.o compass.o tacho.o fxoss.o evt.o  -lpthread -lm

read-tacho: read-tacho.o
read-compass: read-compass.o
merge-tacho-compass: merge-tacho-compass.o
	$(CC) -o merge-tacho-compass merge-tacho-compass.o  -lm
multistream: multistream.c
	$(CC) -o multistream $(CFLAGS) -DSTANDALONE multistream.c
clean:
	rm *.o $(BINPROGS) || true
