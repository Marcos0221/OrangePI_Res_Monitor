CC = gcc
LDFLAGS	= -L/usr/local/lib
LDLIBS = -lwiringPi -lwiringPiDev -lpthread -lm -lcrypt -lrt

res_monitor:	res_monitor.o
	$Q $(CC) -o $@ res_monitor.o $(LDFLAGS) $(LDLIBS)